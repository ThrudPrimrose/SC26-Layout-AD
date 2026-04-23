/*
 * bench_gpu.cu -- loopnest_6 (Nest 25) GPU (CUDA) benchmark.
 * Vertical-only level reduction.
 *
 * Each (blockIdx.y, blockIdx.x) handles one level jk and one je-tile
 * of BX threads; any thread that sees levmask(je, jk) != 0 writes 1.0
 * to lvout[jk]. The write is race-free because all racers write the
 * same value. lvout is cleared by a small init kernel beforehand.
 */
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../../common/gpu_compat.cuh"
#include "bench_common.h"
#include "../loopnest_1/icon_data_loader.h"
#include "../../common/jacobi_flush_gpu.cuh"

/* CUDA_CHECK, WARMUP, NRUNS are provided by the shared headers above. */

#define KARGS_DEV \
  double *__restrict__ lvout, const double *__restrict__ lvm, \
  int N_e, int nlev, int jk0, int jk1

__global__ void gpu_clear(double *lvout, int jk0, int jk1) {
  int jk = blockIdx.x * blockDim.x + threadIdx.x + jk0;
  if (jk < jk1) lvout[jk] = 0.0;
}

template <int V, int BX>
__global__ void gpu_unblocked(KARGS_DEV) {
  int je = blockIdx.x*BX + threadIdx.x;
  int jk = blockIdx.y + jk0;
  if (je >= N_e || jk >= jk1) return;
  if (lvm[IC<V>(je, jk, N_e, nlev)] != 0.0) lvout[jk] = 1.0;
}

template <int B, int BX>
__global__ void gpu_blocked(KARGS_DEV) {
  int je = blockIdx.x*BX + threadIdx.x;
  int jk = blockIdx.y + jk0;
  if (je >= N_e || jk >= jk1) return;
  if (lvm[IC_blocked(je, jk, B, nlev)] != 0.0) lvout[jk] = 1.0;
}

template <int TX, int TY, int BX>
__global__ void gpu_tiled(KARGS_DEV) {
  int je = blockIdx.x*BX + threadIdx.x;
  int jk = blockIdx.y + jk0;
  if (je >= N_e || jk >= jk1) return;
  if (lvm[IC_tiled(je, jk, TX, TY, N_e, nlev)] != 0.0) lvout[jk] = 1.0;
}

struct GCfg { int BX; const char *tag; };
static const GCfg GCFG[] = {
  { 64,  "bx64"}, {128, "bx128"}, {256, "bx256"}, {512, "bx512"}
};
static constexpr int N_GCFG = sizeof(GCFG)/sizeof(GCFG[0]);

#define ARGS_FULL d_lvout,d_lvm,N_e,nlev,jk0,jk1

static void do_clear(double *d_lvout, int jk0, int jk1) {
  int n = jk1 - jk0;
  if (n <= 0) return;
  dim3 T(64,1,1), G((n+63)/64,1,1);
  gpu_clear<<<G,T>>>(d_lvout, jk0, jk1);
}

static void launch_unblocked(int V, int bxi, double *d_lvout, const double *d_lvm,
                             int N_e, int nlev, int jk0, int jk1) {
  int BX = GCFG[bxi].BX;
  int jkn = jk1 - jk0;
  dim3 T(BX, 1, 1);
  dim3 G((N_e + BX - 1)/BX, jkn, 1);
  do_clear(d_lvout, jk0, jk1);
  switch (V*100 + bxi) {
#define CASE_V(V_) \
    case V_*100+0: gpu_unblocked<V_, 64 ><<<G,T>>>(ARGS_FULL); break; \
    case V_*100+1: gpu_unblocked<V_, 128><<<G,T>>>(ARGS_FULL); break; \
    case V_*100+2: gpu_unblocked<V_, 256><<<G,T>>>(ARGS_FULL); break; \
    case V_*100+3: gpu_unblocked<V_, 512><<<G,T>>>(ARGS_FULL); break;
    CASE_V(1) CASE_V(2) CASE_V(3) CASE_V(4)
#undef CASE_V
  }
}

static void launch_blocked(int bi, int bxi, double *d_lvout, const double *d_lvm,
                           int N_e, int nlev, int jk0, int jk1) {
  int BX = GCFG[bxi].BX;
  int jkn = jk1 - jk0;
  dim3 T(BX, 1, 1);
  dim3 G((N_e + BX - 1)/BX, jkn, 1);
  do_clear(d_lvout, jk0, jk1);
  switch (bi*100 + bxi) {
    case 0*100+0: gpu_blocked<8,  64 ><<<G,T>>>(ARGS_FULL); break;
    case 0*100+1: gpu_blocked<8,  128><<<G,T>>>(ARGS_FULL); break;
    case 0*100+2: gpu_blocked<8,  256><<<G,T>>>(ARGS_FULL); break;
    case 0*100+3: gpu_blocked<8,  512><<<G,T>>>(ARGS_FULL); break;
    case 1*100+0: gpu_blocked<16, 64 ><<<G,T>>>(ARGS_FULL); break;
    case 1*100+1: gpu_blocked<16, 128><<<G,T>>>(ARGS_FULL); break;
    case 1*100+2: gpu_blocked<16, 256><<<G,T>>>(ARGS_FULL); break;
    case 1*100+3: gpu_blocked<16, 512><<<G,T>>>(ARGS_FULL); break;
    case 2*100+0: gpu_blocked<32, 64 ><<<G,T>>>(ARGS_FULL); break;
    case 2*100+1: gpu_blocked<32, 128><<<G,T>>>(ARGS_FULL); break;
    case 2*100+2: gpu_blocked<32, 256><<<G,T>>>(ARGS_FULL); break;
    case 2*100+3: gpu_blocked<32, 512><<<G,T>>>(ARGS_FULL); break;
    case 3*100+0: gpu_blocked<64, 64 ><<<G,T>>>(ARGS_FULL); break;
    case 3*100+1: gpu_blocked<64, 128><<<G,T>>>(ARGS_FULL); break;
    case 3*100+2: gpu_blocked<64, 256><<<G,T>>>(ARGS_FULL); break;
    case 3*100+3: gpu_blocked<64, 512><<<G,T>>>(ARGS_FULL); break;
    case 4*100+0: gpu_blocked<128,64 ><<<G,T>>>(ARGS_FULL); break;
    case 4*100+1: gpu_blocked<128,128><<<G,T>>>(ARGS_FULL); break;
    case 4*100+2: gpu_blocked<128,256><<<G,T>>>(ARGS_FULL); break;
    case 4*100+3: gpu_blocked<128,512><<<G,T>>>(ARGS_FULL); break;
  }
}

static void launch_tiled(int txi, int tyi, int bxi, double *d_lvout, const double *d_lvm,
                         int N_e, int nlev, int jk0, int jk1) {
  int BX = GCFG[bxi].BX;
  int jkn = jk1 - jk0;
  dim3 T(BX, 1, 1);
  dim3 G((N_e + BX - 1)/BX, jkn, 1);
  do_clear(d_lvout, jk0, jk1);
#define T4(TX_, TY_) \
  switch (bxi) { \
    case 0: gpu_tiled<TX_,TY_,64 ><<<G,T>>>(ARGS_FULL); break; \
    case 1: gpu_tiled<TX_,TY_,128><<<G,T>>>(ARGS_FULL); break; \
    case 2: gpu_tiled<TX_,TY_,256><<<G,T>>>(ARGS_FULL); break; \
    case 3: gpu_tiled<TX_,TY_,512><<<G,T>>>(ARGS_FULL); break; \
  }
  switch (txi*4 + tyi) {
    case 0*4+0: T4(8,  8)  break; case 0*4+1: T4(8,  16) break; case 0*4+2: T4(8,  32) break; case 0*4+3: T4(8,  64) break;
    case 1*4+0: T4(16, 8)  break; case 1*4+1: T4(16, 16) break; case 1*4+2: T4(16, 32) break; case 1*4+3: T4(16, 64) break;
    case 2*4+0: T4(32, 8)  break; case 2*4+1: T4(32, 16) break; case 2*4+2: T4(32, 32) break; case 2*4+3: T4(32, 64) break;
    case 3*4+0: T4(64, 8)  break; case 3*4+1: T4(64, 16) break; case 3*4+2: T4(64, 32) break; case 3*4+3: T4(64, 64) break;
  }
#undef T4
}

#undef ARGS_FULL
#define ARGS d_lvout, d_lvm, N_e, nlev, jk0, jk1

/* CPU reference: same race-free write pattern as GPU, mirrored as      */
/* lvout[jk] = 1.0 iff any lvm(je, jk) != 0 in [jk0, jk1).               */
template <int V>
static void cpu_ref_unblocked(double *lvout, const double *lvm,
                              int N_e, int nlev, int jk0, int jk1) {
  for (int jk = jk0; jk < jk1; jk++) lvout[jk] = 0.0;
  for (int jk = jk0; jk < jk1; jk++)
    for (int je = 0; je < N_e; je++)
      if (lvm[IC<V>(je, jk, N_e, nlev)] != 0.0) lvout[jk] = 1.0;
}
static void cpu_ref_V(int V, double *lvout, const double *lvm,
                      int N_e, int nlev, int jk0, int jk1) {
  switch (V) {
    case 1: cpu_ref_unblocked<1>(lvout, lvm, N_e, nlev, jk0, jk1); break;
    case 2: cpu_ref_unblocked<2>(lvout, lvm, N_e, nlev, jk0, jk1); break;
    case 3: cpu_ref_unblocked<3>(lvout, lvm, N_e, nlev, jk0, jk1); break;
    case 4: cpu_ref_unblocked<4>(lvout, lvm, N_e, nlev, jk0, jk1); break;
  }
}
static void cpu_ref_blocked(int B, double *lvout, const double *lvm,
                            int N_e, int nlev, int jk0, int jk1) {
  for (int jk = jk0; jk < jk1; jk++) lvout[jk] = 0.0;
  for (int jk = jk0; jk < jk1; jk++)
    for (int je = 0; je < N_e; je++)
      if (lvm[IC_blocked(je, jk, B, nlev)] != 0.0) lvout[jk] = 1.0;
}
static void cpu_ref_tiled(int TX, int TY, double *lvout, const double *lvm,
                          int N_e, int nlev, int jk0, int jk1) {
  for (int jk = jk0; jk < jk1; jk++) lvout[jk] = 0.0;
  for (int jk = jk0; jk < jk1; jk++)
    for (int je = 0; je < N_e; je++)
      if (lvm[IC_tiled(je, jk, TX, TY, N_e, nlev)] != 0.0) lvout[jk] = 1.0;
}

static bool verify_close(const double *got, const double *ref, size_t n,
                         double rtol, double atol, double *max_rel) {
  double mr = 0.0;
  long long nfail = 0;
  for (size_t i = 0; i < n; i++) {
    double diff = std::abs(got[i] - ref[i]);
    double denom = std::max(std::abs(ref[i]), 1e-300);
    double rel = diff / denom;
    if (rel > mr) mr = rel;
    if (diff > atol + rtol * std::abs(ref[i])) nfail++;
  }
  if (max_rel) *max_rel = mr;
  return nfail == 0;
}

static void run_configs_unblocked(FILE *fcsv, int V, BenchData &bd, const char *dist,
    double *d_lvout, const double *d_lvm, cudaEvent_t ev0, cudaEvent_t ev1,
    double *h_ref, double *h_gpu_out) {
  int N_e = bd.N_e, nlev = bd.nlev;
  int jk0 = jk_lo_for(nlev), jk1 = jk_hi_for(nlev);
  cpu_ref_V(V, h_ref, bd.h_lvm, N_e, nlev, jk0, jk1);
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    int BX = GCFG[bxi].BX;
    dim3 T(BX, 1, 1);
    dim3 G((N_e + BX - 1)/BX, (unsigned)(jk1 - jk0), 1);
    if (!gpu_launch_valid(G, T)) {
      printf("SKIP V=%d cfg=%s: invalid launch grid=%ux%u block=%u\n",
             V, GCFG[bxi].tag, G.x, G.y, T.x);
      continue;
    }
    launch_unblocked(V, bxi, ARGS);
    CUDA_CHECK(cudaDeviceSynchronize());
    CUDA_CHECK(cudaMemcpy(h_gpu_out, d_lvout, (size_t)nlev * 8, cudaMemcpyDeviceToHost));
    double mr = 0.0;
    if (!verify_close(h_gpu_out, h_ref, (size_t)nlev, 1e-8, 1e-12, &mr)) {
      printf("FAIL V=%d cfg=%s max_rel=%.3e\n", V, GCFG[bxi].tag, mr);
      continue;
    }
    if (bxi == 0) printf("OK   V=%d dist=%s max_rel=%.3e\n", V, dist, mr);
    for (int r = 0; r < WARMUP; r++) launch_unblocked(V, bxi, ARGS);
    CUDA_CHECK(cudaDeviceSynchronize());
    for (int r = 0; r < NRUNS; r++) {
      flush_jacobi_gpu(); CUDA_CHECK(cudaEventRecord(ev0));
      launch_unblocked(V, bxi, ARGS);
      CUDA_CHECK(cudaEventRecord(ev1));
      CUDA_CHECK(cudaEventSynchronize(ev1));
      float ms; CUDA_CHECK(cudaEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,%d,%d,%d,%s,V%d_%s,run%d,%d,%.6f\n",
              V, nlev, N_e, dist, V, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_blocked(FILE *fcsv, int bi, BenchData &bd, const char *dist,
    double *d_lvout, const double *d_lvm, cudaEvent_t ev0, cudaEvent_t ev1,
    double *h_ref, double *h_gpu_out) {
  int N_e = bd.N_e, nlev = bd.nlev;
  int jk0 = jk_lo_for(nlev), jk1 = jk_hi_for(nlev);
  int B = BLOCK_SIZES[bi];
  cpu_ref_blocked(B, h_ref, bd.h_lvm, N_e, nlev, jk0, jk1);
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    int BX = GCFG[bxi].BX;
    dim3 T(BX, 1, 1);
    dim3 G((N_e + BX - 1)/BX, (unsigned)(jk1 - jk0), 1);
    if (!gpu_launch_valid(G, T)) {
      printf("SKIP B=%d cfg=%s: invalid launch grid=%ux%u block=%u\n",
             B, GCFG[bxi].tag, G.x, G.y, T.x);
      continue;
    }
    launch_blocked(bi, bxi, ARGS);
    CUDA_CHECK(cudaDeviceSynchronize());
    CUDA_CHECK(cudaMemcpy(h_gpu_out, d_lvout, (size_t)nlev * 8, cudaMemcpyDeviceToHost));
    double mr = 0.0;
    if (!verify_close(h_gpu_out, h_ref, (size_t)nlev, 1e-8, 1e-12, &mr)) {
      printf("FAIL B=%d cfg=%s max_rel=%.3e\n", B, GCFG[bxi].tag, mr);
      continue;
    }
    if (bxi == 0) printf("OK   B=%d dist=%s max_rel=%.3e\n", B, dist, mr);
    for (int r = 0; r < WARMUP; r++) launch_blocked(bi, bxi, ARGS);
    CUDA_CHECK(cudaDeviceSynchronize());
    for (int r = 0; r < NRUNS; r++) {
      flush_jacobi_gpu(); CUDA_CHECK(cudaEventRecord(ev0));
      launch_blocked(bi, bxi, ARGS);
      CUDA_CHECK(cudaEventRecord(ev1));
      CUDA_CHECK(cudaEventSynchronize(ev1));
      float ms; CUDA_CHECK(cudaEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,0,%d,%d,%s,B%d_%s,run%d,%d,%.6f\n",
              nlev, N_e, dist, B, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_tiled(FILE *fcsv, int txi, int tyi, BenchData &bd, const char *dist,
    double *d_lvout, const double *d_lvm, cudaEvent_t ev0, cudaEvent_t ev1,
    double *h_ref, double *h_gpu_out) {
  int N_e = bd.N_e, nlev = bd.nlev;
  int jk0 = jk_lo_for(nlev), jk1 = jk_hi_for(nlev);
  int TX = TILE_X_VALUES[txi], TY = TILE_Y_VALUES[tyi + 1];
  cpu_ref_tiled(TX, TY, h_ref, bd.h_lvm, N_e, nlev, jk0, jk1);
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    int BX = GCFG[bxi].BX;
    dim3 T(BX, 1, 1);
    dim3 G((N_e + BX - 1)/BX, (unsigned)(jk1 - jk0), 1);
    if (!gpu_launch_valid(G, T)) {
      printf("SKIP TX=%d TY=%d cfg=%s: invalid launch grid=%ux%u block=%u\n",
             TX, TY, GCFG[bxi].tag, G.x, G.y, T.x);
      continue;
    }
    launch_tiled(txi, tyi, bxi, ARGS);
    CUDA_CHECK(cudaDeviceSynchronize());
    CUDA_CHECK(cudaMemcpy(h_gpu_out, d_lvout, (size_t)nlev * 8, cudaMemcpyDeviceToHost));
    double mr = 0.0;
    if (!verify_close(h_gpu_out, h_ref, (size_t)nlev, 1e-8, 1e-12, &mr)) {
      printf("FAIL TX=%d TY=%d cfg=%s max_rel=%.3e\n", TX, TY, GCFG[bxi].tag, mr);
      continue;
    }
    if (bxi == 0) printf("OK   TX=%d TY=%d dist=%s max_rel=%.3e\n", TX, TY, dist, mr);
    for (int r = 0; r < WARMUP; r++) launch_tiled(txi, tyi, bxi, ARGS);
    CUDA_CHECK(cudaDeviceSynchronize());
    for (int r = 0; r < NRUNS; r++) {
      flush_jacobi_gpu(); CUDA_CHECK(cudaEventRecord(ev0));
      launch_tiled(txi, tyi, bxi, ARGS);
      CUDA_CHECK(cudaEventRecord(ev1));
      CUDA_CHECK(cudaEventSynchronize(ev1));
      float ms; CUDA_CHECK(cudaEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,0,%d,%d,%s,T%dx%d_%s,run%d,%d,%.6f\n",
              nlev, N_e, dist, TX, TY, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}

int main(int argc, char *argv[]) {
  const char *csv = (argc >= 2) ? argv[1] : "levelmask_gpu.csv";
  FILE *fcsv = fopen(csv, "w"); if (!fcsv) { perror("fopen"); return 1; }
  fprintf(fcsv, "backend,V,nlev,N_e,cell_dist,config_label,run_label,run_id,time_ms\n");

  int step = (argc >= 3) ? atoi(argv[2]) : 9;
  std::string gp = icon_global_path(step), pp = icon_patch_path(step);
  int icon_nproma = icon_read_nproma(gp.c_str());
  IconEdgeData ied;
  bool have_exact = (icon_nproma > 0) && icon_load_patch(pp.c_str(), icon_nproma, ied);
  int N_e = have_exact ? ied.n_edges : 122880;

  int max_nlev = 0;
  for (int ni = 0; ni < N_NLEVS; ni++) if (NLEVS[ni] > max_nlev) max_nlev = NLEVS[ni];
  size_t sz_max = (size_t)N_e * max_nlev;
  double *d_lvm, *d_lvout;
  CUDA_CHECK(cudaMalloc(&d_lvm, sz_max*8));
  CUDA_CHECK(cudaMalloc(&d_lvout, (size_t)max_nlev*8));
  std::vector<double> h_ref(max_nlev), h_gpu_out(max_nlev);
  cudaEvent_t ev0, ev1; CUDA_CHECK(cudaEventCreate(&ev0)); CUDA_CHECK(cudaEventCreate(&ev1));

  const char *dists[3] = {"uniform", "normal_var1", "exact"};
  int ndists = have_exact ? 3 : 2;

  for (int ni = 0; ni < N_NLEVS; ni++) {
    int nlev = NLEVS[ni];
    size_t sz = (size_t)N_e * nlev;

    for (int di = 0; di < ndists; di++) {
      for (int V = 1; V <= 4; V++) {
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant(V);
        CUDA_CHECK(cudaMemcpy(d_lvm, bd.h_lvm, sz*8, cudaMemcpyHostToDevice));
        run_configs_unblocked(fcsv, V, bd, dists[di], d_lvout, d_lvm, ev0, ev1,
                              h_ref.data(), h_gpu_out.data());
        fflush(fcsv); bd.free_all();
      }
    }

    for (int di = 0; di < ndists; di++) {
      for (int bi = 0; bi < N_BLOCK_SIZES; bi++) {
        int B = BLOCK_SIZES[bi];
        if (N_e % B != 0) { printf("SKIP GPU B=%d\n", B); continue; }
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant_blocked(B);
        CUDA_CHECK(cudaMemcpy(d_lvm, bd.h_lvm, sz*8, cudaMemcpyHostToDevice));
        run_configs_blocked(fcsv, bi, bd, dists[di], d_lvout, d_lvm, ev0, ev1,
                            h_ref.data(), h_gpu_out.data());
        fflush(fcsv); bd.free_all();
      }
    }

    for (int di = 0; di < ndists; di++) {
      for (int txi = 0; txi < N_TILE_X; txi++) {
        int TX = TILE_X_VALUES[txi];
        if (N_e % TX != 0) continue;
        for (int tyi = 0; tyi < 4; tyi++) {
          int TY = TILE_Y_VALUES[tyi + 1];
          if (nlev % TY != 0) continue;
          BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
          bd.set_variant_tiled(TX, TY);
          CUDA_CHECK(cudaMemcpy(d_lvm, bd.h_lvm, sz*8, cudaMemcpyHostToDevice));
          run_configs_tiled(fcsv, txi, tyi, bd, dists[di], d_lvout, d_lvm, ev0, ev1,
                            h_ref.data(), h_gpu_out.data());
          fflush(fcsv); bd.free_all();
        }
      }
    }
  }

  CUDA_CHECK(cudaFree(d_lvm)); CUDA_CHECK(cudaFree(d_lvout));
  CUDA_CHECK(cudaEventDestroy(ev0)); CUDA_CHECK(cudaEventDestroy(ev1));
  if (have_exact) ied.free_all();
  fclose(fcsv);
  return 0;
}
