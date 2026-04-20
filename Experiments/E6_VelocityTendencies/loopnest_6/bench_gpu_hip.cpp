/*
 * bench_gpu_hip.cpp -- loopnest_6 (Nest 25) GPU (HIP) benchmark.
 * Vertical-only level reduction.
 */
#include "hip/hip_runtime.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "bench_common.h"
#include "../loopnest_1/icon_data_loader.h"

#define HIP_CHECK(x) do { hipError_t e=(x); \
  if(e!=hipSuccess){fprintf(stderr,"HIP err %s:%d %s\n",__FILE__,__LINE__,hipGetErrorString(e));std::abort();} } while(0)

static constexpr int GPU_WARMUP = 5;
static constexpr int GPU_NRUNS  = 100;

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

static void run_configs_unblocked(FILE *fcsv, int V, BenchData &bd, const char *dist,
    double *d_lvout, const double *d_lvm, hipEvent_t ev0, hipEvent_t ev1) {
  int N_e = bd.N_e, nlev = bd.nlev;
  int jk0 = jk_lo_for(nlev), jk1 = jk_hi_for(nlev);
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++) launch_unblocked(V, bxi, ARGS);
    HIP_CHECK(hipDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      HIP_CHECK(hipEventRecord(ev0, 0));
      launch_unblocked(V, bxi, ARGS);
      HIP_CHECK(hipEventRecord(ev1, 0));
      HIP_CHECK(hipEventSynchronize(ev1));
      float ms; HIP_CHECK(hipEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,%d,%d,%d,%s,V%d_%s,run%d,%d,%.6f\n",
              V, nlev, N_e, dist, V, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_blocked(FILE *fcsv, int bi, BenchData &bd, const char *dist,
    double *d_lvout, const double *d_lvm, hipEvent_t ev0, hipEvent_t ev1) {
  int N_e = bd.N_e, nlev = bd.nlev;
  int jk0 = jk_lo_for(nlev), jk1 = jk_hi_for(nlev);
  int B = BLOCK_SIZES[bi];
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++) launch_blocked(bi, bxi, ARGS);
    HIP_CHECK(hipDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      HIP_CHECK(hipEventRecord(ev0, 0));
      launch_blocked(bi, bxi, ARGS);
      HIP_CHECK(hipEventRecord(ev1, 0));
      HIP_CHECK(hipEventSynchronize(ev1));
      float ms; HIP_CHECK(hipEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,0,%d,%d,%s,B%d_%s,run%d,%d,%.6f\n",
              nlev, N_e, dist, B, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_tiled(FILE *fcsv, int txi, int tyi, BenchData &bd, const char *dist,
    double *d_lvout, const double *d_lvm, hipEvent_t ev0, hipEvent_t ev1) {
  int N_e = bd.N_e, nlev = bd.nlev;
  int jk0 = jk_lo_for(nlev), jk1 = jk_hi_for(nlev);
  int TX = TILE_X_VALUES[txi], TY = TILE_Y_VALUES[tyi + 1];
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++) launch_tiled(txi, tyi, bxi, ARGS);
    HIP_CHECK(hipDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      HIP_CHECK(hipEventRecord(ev0, 0));
      launch_tiled(txi, tyi, bxi, ARGS);
      HIP_CHECK(hipEventRecord(ev1, 0));
      HIP_CHECK(hipEventSynchronize(ev1));
      float ms; HIP_CHECK(hipEventElapsedTime(&ms, ev0, ev1));
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

  size_t sz = (size_t)N_e * NLEVS[0];
  double *d_lvm, *d_lvout;
  HIP_CHECK(hipMalloc(&d_lvm, sz*8));
  HIP_CHECK(hipMalloc(&d_lvout, (size_t)NLEVS[0]*8));
  hipEvent_t ev0, ev1; hipEventCreate(&ev0); hipEventCreate(&ev1);

  const char *dists[3] = {"uniform", "normal_var1", "exact"};
  int ndists = have_exact ? 3 : 2;

  for (int ni = 0; ni < N_NLEVS; ni++) {
    int nlev = NLEVS[ni];

    for (int di = 0; di < ndists; di++) {
      for (int V = 1; V <= 4; V++) {
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant(V);
        HIP_CHECK(hipMemcpy(d_lvm, bd.h_lvm, sz*8, hipMemcpyHostToDevice));
        run_configs_unblocked(fcsv, V, bd, dists[di], d_lvout, d_lvm, ev0, ev1);
        fflush(fcsv); bd.free_all();
      }
    }

    for (int di = 0; di < ndists; di++) {
      for (int bi = 0; bi < N_BLOCK_SIZES; bi++) {
        int B = BLOCK_SIZES[bi];
        if (N_e % B != 0) { printf("SKIP GPU B=%d\n", B); continue; }
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant_blocked(B);
        HIP_CHECK(hipMemcpy(d_lvm, bd.h_lvm, sz*8, hipMemcpyHostToDevice));
        run_configs_blocked(fcsv, bi, bd, dists[di], d_lvout, d_lvm, ev0, ev1);
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
          HIP_CHECK(hipMemcpy(d_lvm, bd.h_lvm, sz*8, hipMemcpyHostToDevice));
          run_configs_tiled(fcsv, txi, tyi, bd, dists[di], d_lvout, d_lvm, ev0, ev1);
          fflush(fcsv); bd.free_all();
        }
      }
    }
  }

  hipFree(d_lvm); hipFree(d_lvout);
  hipEventDestroy(ev0); hipEventDestroy(ev1);
  if (have_exact) ied.free_all();
  fclose(fcsv);
  return 0;
}
