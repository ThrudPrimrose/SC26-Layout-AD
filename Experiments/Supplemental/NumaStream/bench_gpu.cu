/*
 * bench_gpu.cu -- GPU sweep for C = alpha * (A + B) over very large
 *                 fp64 matrices. Sweeps thread-block size and grid
 *                 occupancy to measure achieved bandwidth across
 *                 launch-parameter choices.
 *
 * Kernel:  C[i] = alpha * (A[i] + B[i])
 * Traffic: 2 loads + 1 store per fp64 = 24 bytes per element.
 * Default: N values {8192, 16384, 24576}; buffer sizes 512 MiB, 2 GiB,
 *          4.5 GiB each -- A, B, C allocated simultaneously.
 *
 * Launch-parameter sweep:
 *   block sizes:  {64, 128, 256, 512, 1024}
 *   grid sizes:   {sm_count * m}  for m in {2, 4, 8, 16, 32}
 *     (grid-stride loop, so any m >= 1 covers the domain; larger m
 *     trades occupancy for scheduling overhead).
 *
 * Same source compiles for both CUDA (nvcc) and HIP (hipcc); the thin
 * shim at the top maps cuda/hip symbols to a neutral "gpu*" alias.
 *
 * Between every timed iteration we run a GPU-side cache-buster on the
 * device (a simple write pass over a 1 GiB scratch buffer) so the sweep
 * starts every rep with cold L2. We also run 5 warmups before the
 * first timed rep of each (block, grid) combination.
 *
 * Output CSV header:
 *   device,N,elems,bytes_per_iter,block,grid_mult,grid,reps,bw_gbs,bw_tbs,alpha,sm_count
 *
 * Usage:
 *   bench_gpu [out.csv=results/numa_gpu.csv]
 *             [NLIST="8192,16384,24576"]
 *             [reps=50]
 *             [alpha=1.0001]
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../../common/gpu_compat.cuh"     /* cuda/hip unification */
#define CK GPU_CHECK

/* Canonical GPU Jacobi flush -- 8192^2, 3 swept steps with buffer swap.
 * Lives in common/jacobi_flush_gpu.cuh alongside the CPU twin. Between
 * every timed rep we invoke flush_jacobi_gpu() for parity with every
 * other bench in the artifact. */
#include "../../common/jacobi_flush_gpu.cuh"

/* ------------------------------------------------------------------ */
/*  Kernels                                                            */
/* ------------------------------------------------------------------ */
__global__ void scale_add(double *__restrict__ C,
                          const double *__restrict__ A,
                          const double *__restrict__ B,
                          size_t N, double alpha) {
  size_t i = (size_t)blockIdx.x * blockDim.x + threadIdx.x;
  size_t stride = (size_t)gridDim.x * blockDim.x;
  for (; i < N; i += stride) C[i] = alpha * (A[i] + B[i]);
}

__global__ void fill(double *__restrict__ buf, size_t N, double v) {
  size_t i = (size_t)blockIdx.x * blockDim.x + threadIdx.x;
  size_t stride = (size_t)gridDim.x * blockDim.x;
  for (; i < N; i += stride) buf[i] = v;
}

/* ------------------------------------------------------------------ */
/*  Sweep configuration                                                */
/* ------------------------------------------------------------------ */
static const int BLOCK_SIZES[] = { 64, 128, 256, 512, 1024 };
static const int N_BLOCKS      = sizeof(BLOCK_SIZES) / sizeof(BLOCK_SIZES[0]);
static const int GRID_MULTS[]  = { 2, 4, 8, 16, 32 };
static const int N_GRIDS       = sizeof(GRID_MULTS) / sizeof(GRID_MULTS[0]);

/* Repo-wide iteration convention (matches every E1-E6 bench). */
static constexpr int WARMUP_DEFAULT = 5;
static constexpr int NRUNS_DEFAULT  = 100;

/* ------------------------------------------------------------------ */
/*  Timed run for one (block, grid_mult) combination                   */
/*                                                                     */
/*  WARMUP untimed iterations, then NRUNS timed iterations with the    */
/*  canonical GPU Jacobi flush (8192^2, 3 swept sweeps) between every  */
/*  rep. The full per-rep bandwidth vector is written back through     */
/*  `per_rep_bps_out` so the CSV can carry the whole distribution.     */
/* ------------------------------------------------------------------ */
static double time_launch(double *__restrict__ A,
                          double *__restrict__ B,
                          double *__restrict__ C,
                          size_t N, int block, int grid,
                          int warmup, int nruns, double alpha,
                          gpuEvent_t e0, gpuEvent_t e1,
                          std::vector<double> &per_rep_bps_out) {
  /* warmup (also warms up the Jacobi flush buffers on first call). */
  for (int w = 0; w < warmup; ++w) {
    flush_jacobi_gpu();
    scale_add<<<grid, block>>>(C, A, B, N, alpha);
  }
  CK(gpuDeviceSynchronize());

  per_rep_bps_out.clear();
  per_rep_bps_out.reserve(nruns);
  for (int r = 0; r < nruns; ++r) {
    flush_jacobi_gpu();
    CK(gpuEventRecord(e0));
    scale_add<<<grid, block>>>(C, A, B, N, alpha);
    CK(gpuEventRecord(e1));
    CK(gpuEventSynchronize(e1));
    float ms = 0;
    CK(gpuEventElapsedTime(&ms, e0, e1));
    double bps = 3.0 * (double)N * sizeof(double) / (ms * 1e-3);
    per_rep_bps_out.push_back(bps);
  }

  std::vector<double> sorted = per_rep_bps_out;
  std::sort(sorted.begin(), sorted.end());
  return sorted[sorted.size() / 2];        /* median bytes / s */
}

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */
static std::vector<size_t> parse_nlist(const char *spec) {
  std::vector<size_t> v;
  const char *p = spec;
  while (*p) {
    char *e = nullptr;
    long long x = strtoll(p, &e, 10);
    if (e == p) break;
    if (x > 0) v.push_back((size_t)x);
    p = e;
    while (*p == ',' || *p == ' ') ++p;
  }
  return v;
}

int main(int argc, char **argv) {
  const char *out    = (argc > 1) ? argv[1] : "results/numa_gpu.csv";
  const char *nlist  = (argc > 2) ? argv[2] : "8192,16384,24576";
  int         warmup = (argc > 3) ? atoi(argv[3]) : WARMUP_DEFAULT;
  int         nruns  = (argc > 4) ? atoi(argv[4]) : NRUNS_DEFAULT;
  double      alpha  = (argc > 5) ? atof(argv[5]) : 1.0001;

  gpuDeviceProp prop;
  CK(gpuGetDeviceProperties(&prop, 0));
  int sm_count = prop.multiProcessorCount;

  std::vector<size_t> sizes = parse_nlist(nlist);
  if (sizes.empty()) sizes = { 8192, 16384, 24576 };

  fprintf(stderr, "[bench_numa gpu] device=%s  SMs=%d\n"
                  "                 warmup=%d  nruns=%d  alpha=%.6f\n",
          prop.name, sm_count, warmup, nruns, alpha);

  /* Allocate at the largest N and reuse across sizes via fill. */
  size_t N1d_max = sizes[0];
  for (size_t n : sizes) if (n > N1d_max) N1d_max = n;
  size_t N_max = N1d_max * N1d_max;

  double *A = nullptr, *B = nullptr, *C = nullptr;
  CK(gpuMalloc((void **)&A, N_max * sizeof(double)));
  CK(gpuMalloc((void **)&B, N_max * sizeof(double)));
  CK(gpuMalloc((void **)&C, N_max * sizeof(double)));

  /* One-time init of the canonical GPU Jacobi flush (see
   * common/jacobi_flush_gpu.cuh). Subsequent flush calls reuse these. */
  flush_jacobi_gpu_init();

  gpuEvent_t e0, e1;
  CK(gpuEventCreate(&e0));
  CK(gpuEventCreate(&e1));

  FILE *f = fopen(out, "w"); if (!f) { perror("fopen"); return 1; }
  /* "Tall" schema: one row per rep so plotting scripts can reconstruct
   * the full empirical distribution without bespoke aggregation. */
  fprintf(f, "device,N,elems,bytes_per_iter,block,grid_mult,grid,"
             "warmup,nruns,run_id,bw_gbs,bw_tbs,alpha,sm_count\n");

  std::vector<double> per_rep;

  for (size_t N1d : sizes) {
    size_t N = N1d * N1d;
    size_t bytes_per_iter = 3 * N * sizeof(double);

    fprintf(stderr, "[bench_numa gpu] -- N=%zu (%.2f GiB/buf) --\n",
            N1d, N * sizeof(double) / (double)(1UL << 30));

    /* Fill inputs; the output is overwritten on every rep. */
    fill<<<4096, 256>>>(A, N, 1.0);
    fill<<<4096, 256>>>(B, N, 2.0);
    fill<<<4096, 256>>>(C, N, 0.0);
    CK(gpuDeviceSynchronize());

    for (int bi = 0; bi < N_BLOCKS; ++bi) {
      int block = BLOCK_SIZES[bi];
      for (int gi = 0; gi < N_GRIDS; ++gi) {
        int grid_mult = GRID_MULTS[gi];
        int grid = sm_count * grid_mult;

        double bps_median = time_launch(A, B, C, N, block, grid,
                                        warmup, nruns, alpha,
                                        e0, e1, per_rep);
        fprintf(stderr, "[bench_numa gpu]   block=%-4d grid=%dx%d=%-6d  median %.2f GB/s\n",
                block, sm_count, grid_mult, grid, bps_median * 1e-9);

        for (int r = 0; r < (int)per_rep.size(); ++r) {
          double bps = per_rep[r];
          double gbs = bps * 1e-9, tbs = bps * 1e-12;
          fprintf(f, "gpu,%zu,%zu,%zu,%d,%d,%d,%d,%d,%d,%.4f,%.6f,%.6f,%d\n",
                  N1d, N, bytes_per_iter, block, grid_mult, grid,
                  warmup, nruns, r, gbs, tbs, alpha, sm_count);
        }
        fflush(f);
      }
    }
  }

  fclose(f);
  CK(gpuEventDestroy(e0));
  CK(gpuEventDestroy(e1));
  flush_jacobi_gpu_destroy();
  CK(gpuFree(A));
  CK(gpuFree(B));
  CK(gpuFree(C));
  return 0;
}
