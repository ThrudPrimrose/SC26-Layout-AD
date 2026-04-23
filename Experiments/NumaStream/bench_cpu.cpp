/*
 * bench_cpu.cpp -- NUMA-aware CPU sweep for C = alpha * (A + B)
 *                   over very large fp64 matrices.
 *
 * Kernel:   C[i] = alpha * (A[i] + B[i])
 * Traffic:  2 loads + 1 store per fp64 element = 24 bytes per element.
 * Default:  three N values: 8192, 16384, 24576.
 *           (16384^2 fp64 = 2 GiB per buffer; 24576^2 = 4.5 GiB per buffer).
 *
 * Variants (one CSV row per (variant, N) combination):
 *
 *   baseline_ft       -- plain mmap + MADV_HUGEPAGE + parallel
 *                        first-touch. No explicit mbind; pages land
 *                        wherever OMP_PROC_BIND's first-touching thread
 *                        sits (this is what every other bench in the
 *                        repo uses by default).
 *   numa{D}_stripe    -- D contiguous row-stripes (D = min(detected
 *                        nodes, 4)), each mbind'd to its own NUMA node
 *                        and first-touched in parallel. This is what
 *                        the paper refers to as "tile everything into 4
 *                        NUMA domains" for Grace and Zen4.
 *   interleave        -- MPOL_INTERLEAVE across all nodes (page-cyclic,
 *                        the kernel-default fallback when threads are
 *                        not well pinned). Kept as a comparison point.
 *
 * The NUMA primitives (numa_alloc, bind_and_touch, numa_page_size) live
 * in ../../common/numa_util.h so the per-page policy is identical to
 * E1-E6.
 *
 * Output CSV header:
 *   device,variant,N,elems,bytes_per_iter,threads,numa_nodes,reps,bw_gbs,bw_tbs,alpha
 *
 * Usage:
 *   bench_cpu [out.csv=results/numa_cpu.csv]
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

#include <omp.h>
#include <numaif.h>

#include "../../common/numa_util.h"
#include "../../common/jacobi_flush.h"

/* ------------------------------------------------------------------ */
/*  Kernel                                                             */
/* ------------------------------------------------------------------ */
static inline void scale_add(double       *__restrict__ C,
                             const double *__restrict__ A,
                             const double *__restrict__ B,
                             size_t lo, size_t hi, double alpha) {
  for (size_t i = lo; i < hi; ++i) C[i] = alpha * (A[i] + B[i]);
}

/* ------------------------------------------------------------------ */
/*  Common timed loop                                                  */
/*                                                                     */
/*  Matches the repo-wide convention used by every E1-E6 bench:        */
/*    WARMUP untimed iterations, then NRUNS timed iterations with the  */
/*    canonical 8192^2 Jacobi flush running between every rep.         */
/*                                                                     */
/*  Returns the *median* bandwidth across the NRUNS timed iterations   */
/*  (not the minimum), so outlier behaviour is preserved in line with  */
/*  the "no outlier trimming" policy documented in common/plot_util.py.*/
/* ------------------------------------------------------------------ */
static constexpr int WARMUP_DEFAULT = 5;
static constexpr int NRUNS_DEFAULT  = 100;

static double time_kernel(double *__restrict__ A,
                          double *__restrict__ B,
                          double *__restrict__ C,
                          size_t N, int warmup, int nruns, double alpha,
                          std::vector<double> *per_rep_bps_out = nullptr) {
  /* warmup */
  for (int w = 0; w < warmup; ++w) {
    flush_jacobi();
    #pragma omp parallel
    {
      int tid = omp_get_thread_num();
      int nth = omp_get_num_threads();
      size_t lo = (size_t)N * tid / nth;
      size_t hi = (size_t)N * (tid + 1) / nth;
      scale_add(C, A, B, lo, hi, alpha);
    }
  }

  std::vector<double> per_rep_bps;
  per_rep_bps.reserve(nruns);
  for (int r = 0; r < nruns; ++r) {
    flush_jacobi();                       /* canonical 8192^2 Jacobi */

    double t0 = 0, t1 = 0;
    #pragma omp parallel
    {
      #pragma omp barrier
      #pragma omp master
      t0 = omp_get_wtime();
      #pragma omp barrier

      int tid = omp_get_thread_num();
      int nth = omp_get_num_threads();
      size_t lo = (size_t)N * tid / nth;
      size_t hi = (size_t)N * (tid + 1) / nth;
      scale_add(C, A, B, lo, hi, alpha);

      #pragma omp barrier
      #pragma omp master
      t1 = omp_get_wtime();
    }
    double dt = t1 - t0;
    per_rep_bps.push_back(3.0 * (double)N * sizeof(double) / dt);
  }

  if (per_rep_bps_out) *per_rep_bps_out = per_rep_bps;

  std::vector<double> sorted = per_rep_bps;
  std::sort(sorted.begin(), sorted.end());
  return sorted[sorted.size() / 2];        /* median bytes / s */
}

/* ------------------------------------------------------------------ */
/*  Variant: plain first-touch, no explicit binding                    */
/* ------------------------------------------------------------------ */
static double run_baseline(double *__restrict__ A,
                           double *__restrict__ B,
                           double *__restrict__ C,
                           size_t N, int warmup, int nruns, double alpha,
                           std::vector<double> *per_rep) {
  #pragma omp parallel for schedule(static)
  for (size_t i = 0; i < N; ++i) { A[i] = 1.0; B[i] = 2.0; C[i] = 0.0; }
  return time_kernel(A, B, C, N, warmup, nruns, alpha, per_rep);
}

/* ------------------------------------------------------------------ */
/*  Variant: explicit D-NUMA-stripe via bind_and_touch                 */
/* ------------------------------------------------------------------ */
static double run_numa_stripe(double *__restrict__ A,
                              double *__restrict__ B,
                              double *__restrict__ C,
                              size_t N, int warmup, int nruns, double alpha,
                              std::vector<double> *per_rep) {
  /* bind_and_touch partitions the buffer into D contiguous pages and
   * mbinds each chunk to one node, then first-touches every page. */
  bind_and_touch(A, N * sizeof(double));
  bind_and_touch(B, N * sizeof(double));
  bind_and_touch(C, N * sizeof(double));

  #pragma omp parallel for schedule(static)
  for (size_t i = 0; i < N; ++i) { A[i] = 1.0; B[i] = 2.0; C[i] = 0.0; }
  return time_kernel(A, B, C, N, warmup, nruns, alpha, per_rep);
}

/* ------------------------------------------------------------------ */
/*  Variant: MPOL_INTERLEAVE                                           */
/* ------------------------------------------------------------------ */
static void mpol_interleave(void *base, size_t bytes, int D) {
  const size_t ps = numa_page_size();
  unsigned long mask = 0;
  for (int d = 0; d < D; ++d) mask |= (1UL << d);
  size_t pages = (bytes + ps - 1) / ps;
  mbind(base, pages * ps, MPOL_INTERLEAVE, &mask, 64, 0);
  #pragma omp parallel for schedule(static)
  for (size_t i = 0; i < bytes; i += ps) {
    volatile char *p = reinterpret_cast<volatile char *>(base) + i;
    *p = 0;
  }
}

static double run_interleave(double *__restrict__ A,
                             double *__restrict__ B,
                             double *__restrict__ C,
                             size_t N, int D, int warmup, int nruns, double alpha,
                             std::vector<double> *per_rep) {
  mpol_interleave(A, N * sizeof(double), D);
  mpol_interleave(B, N * sizeof(double), D);
  mpol_interleave(C, N * sizeof(double), D);

  #pragma omp parallel for schedule(static)
  for (size_t i = 0; i < N; ++i) { A[i] = 1.0; B[i] = 2.0; C[i] = 0.0; }
  return time_kernel(A, B, C, N, warmup, nruns, alpha, per_rep);
}

/* ------------------------------------------------------------------ */
/*  helpers                                                            */
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

/* One CSV row per timed rep, so that plotting scripts can reconstruct
 * the whole empirical distribution (violin, percentile, whatever) and
 * not just the aggregated median. Column schema is the "tall" form
 * used by plot_util.load_csv(). */
static void emit_rows(FILE *f, const char *variant, size_t N1d,
                      size_t elems, size_t bytes_per_iter,
                      int P, int D, int warmup, int nruns,
                      const std::vector<double> &per_rep_bps,
                      double alpha) {
  for (int r = 0; r < (int)per_rep_bps.size(); ++r) {
    double bps = per_rep_bps[r];
    double gbs = bps * 1e-9, tbs = bps * 1e-12;
    fprintf(f,
            "cpu,%s,%zu,%zu,%zu,%d,%d,%d,%d,%d,%.4f,%.6f,%.6f\n",
            variant, N1d, elems, bytes_per_iter, P, D, warmup, nruns, r,
            gbs, tbs, alpha);
  }
  fflush(f);
}

/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */
int main(int argc, char **argv) {
  const char *out    = (argc > 1) ? argv[1] : "results/numa_cpu.csv";
  const char *nlist  = (argc > 2) ? argv[2] : "8192,16384,24576";
  int        warmup  = (argc > 3) ? atoi(argv[3]) : WARMUP_DEFAULT;
  int        nruns   = (argc > 4) ? atoi(argv[4]) : NRUNS_DEFAULT;
  double     alpha   = (argc > 5) ? atof(argv[5]) : 1.0001;

  int P = omp_get_max_threads();
  int D = numa_num_nodes();
  int tile_D = (D < 4) ? D : 4;

  std::vector<size_t> sizes = parse_nlist(nlist);
  if (sizes.empty()) sizes = { 8192, 16384, 24576 };

  fprintf(stderr, "[bench_numa cpu] threads=%d  numa_nodes=%d  tile_D=%d\n"
                  "                 warmup=%d  nruns=%d  alpha=%.6f\n",
          P, D, tile_D, warmup, nruns, alpha);

  /* allocate once at the largest N and reuse across variants via
   * re-touch; this keeps the bench self-contained and avoids any
   * cross-run NUMA hangover. */
  size_t N1d_max = *std::max_element(sizes.begin(), sizes.end());
  size_t N_max   = N1d_max * N1d_max;

  double *A = numa_alloc_unfaulted<double>(N_max);
  double *B = numa_alloc_unfaulted<double>(N_max);
  double *C = numa_alloc_unfaulted<double>(N_max);

  flush_jacobi_init();                    /* serial alloc of flush buffers */

  FILE *f = fopen(out, "w"); if (!f) { perror("fopen"); return 1; }
  /* "Tall" schema: one row per rep. Discovered by common/plot_util.py. */
  fprintf(f, "device,variant,N,elems,bytes_per_iter,threads,numa_nodes,"
             "warmup,nruns,run_id,bw_gbs,bw_tbs,alpha\n");

  char vtile[32];
  std::snprintf(vtile, sizeof(vtile), "numa%d_stripe", tile_D);

  for (size_t N1d : sizes) {
    size_t N = N1d * N1d;
    size_t bytes_per_iter = 3 * N * sizeof(double);

    fprintf(stderr, "[bench_numa cpu] -- N=%zu (%.2f GiB/buf) --\n",
            N1d, N * sizeof(double) / (double)(1UL << 30));

    std::vector<double> per_rep;
    double bps;

    bps = run_baseline(A, B, C, N, warmup, nruns, alpha, &per_rep);
    fprintf(stderr, "[bench_numa cpu]   baseline_ft     median %.2f GB/s\n", bps * 1e-9);
    emit_rows(f, "baseline_ft", N1d, N, bytes_per_iter, P, D, warmup, nruns, per_rep, alpha);

    if (D >= 2) {
      bps = run_numa_stripe(A, B, C, N, warmup, nruns, alpha, &per_rep);
      fprintf(stderr, "[bench_numa cpu]   %-14s median %.2f GB/s\n", vtile, bps * 1e-9);
      emit_rows(f, vtile, N1d, N, bytes_per_iter, P, D, warmup, nruns, per_rep, alpha);

      bps = run_interleave(A, B, C, N, D, warmup, nruns, alpha, &per_rep);
      fprintf(stderr, "[bench_numa cpu]   interleave      median %.2f GB/s\n", bps * 1e-9);
      emit_rows(f, "interleave", N1d, N, bytes_per_iter, P, D, warmup, nruns, per_rep, alpha);
    } else {
      fprintf(stderr, "[bench_numa cpu]   (numaD_stripe and interleave skipped: D=%d)\n", D);
    }
  }

  fclose(f);
  numa_dealloc(A, N_max);
  numa_dealloc(B, N_max);
  numa_dealloc(C, N_max);
  return 0;
}
