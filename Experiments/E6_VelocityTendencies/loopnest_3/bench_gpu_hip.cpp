/*
 * bench_gpu_hip.cpp -- loopnest_3 (Nest 7) GPU (HIP) benchmark.
 * HIP-compatible mirror of bench_gpu.cu.
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

template <int V, int BX, int BY>
__global__ void gpu_unblocked(
    double *__restrict__ w_io, const double *__restrict__ vn_ie, const double *__restrict__ z_vt_ie,
    const double *__restrict__ gradh, const double *__restrict__ invr,
    const double *__restrict__ ft_e,  const double *__restrict__ fn_e,
    int N_e, int nlev, int jk0, int jk1) {
  int je = blockIdx.x*BX + threadIdx.x;
  int jk = blockIdx.y*BY + threadIdx.y;
  if (je >= N_e || jk < jk0 || jk >= jk1) return;
  int c = IC<V>(je, jk, N_e, nlev);
  double w = w_io[c], vie = vn_ie[c], vti = z_vt_ie[c];
  double iv = invr[jk];
  w_io[c] = w*gradh[jk] + vie*(vie*iv - ft_e[je]) + vti*(vti*iv + fn_e[je]);
}

template <int B, int BX, int BY>
__global__ void gpu_blocked(
    double *__restrict__ w_io, const double *__restrict__ vn_ie, const double *__restrict__ z_vt_ie,
    const double *__restrict__ gradh, const double *__restrict__ invr,
    const double *__restrict__ ft_e,  const double *__restrict__ fn_e,
    int N_e, int nlev, int jk0, int jk1) {
  int je = blockIdx.x*BX + threadIdx.x;
  int jk = blockIdx.y*BY + threadIdx.y;
  if (je >= N_e || jk < jk0 || jk >= jk1) return;
  int c = IC_blocked(je, jk, B, nlev);
  double w = w_io[c], vie = vn_ie[c], vti = z_vt_ie[c];
  double iv = invr[jk];
  w_io[c] = w*gradh[jk] + vie*(vie*iv - ft_e[je]) + vti*(vti*iv + fn_e[je]);
}

template <int TX, int TY, int BX, int BY>
__global__ void gpu_tiled(
    double *__restrict__ w_io, const double *__restrict__ vn_ie, const double *__restrict__ z_vt_ie,
    const double *__restrict__ gradh, const double *__restrict__ invr,
    const double *__restrict__ ft_e,  const double *__restrict__ fn_e,
    int N_e, int nlev, int jk0, int jk1) {
  int je = blockIdx.x*BX + threadIdx.x;
  int jk = blockIdx.y*BY + threadIdx.y;
  if (je >= N_e || jk < jk0 || jk >= jk1) return;
  int c = IC_tiled(je, jk, TX, TY, N_e, nlev);
  double w = w_io[c], vie = vn_ie[c], vti = z_vt_ie[c];
  double iv = invr[jk];
  w_io[c] = w*gradh[jk] + vie*(vie*iv - ft_e[je]) + vti*(vti*iv + fn_e[je]);
}

struct GCfg { int BX, BY; const char *tag; };
static const GCfg GCFG[] = {
  { 32,  4, "bx32_by04"},
  { 32,  8, "bx32_by08"},
  { 64,  4, "bx64_by04"},
  { 16, 16, "bx16_by16"},
  {128,  1, "bx128_by01"}
};
static constexpr int N_GCFG = sizeof(GCFG)/sizeof(GCFG[0]);

#define ARGS d_w,d_vie,d_vti,d_gh,d_iv,d_ft,d_fn,N_e,nlev,jk0,jk1

static void launch_unblocked(int V, int bxi, double *d_w, const double *d_vie,
                             const double *d_vti, const double *d_gh, const double *d_iv,
                             const double *d_ft, const double *d_fn,
                             int N_e, int nlev, int jk0, int jk1) {
  int BX = GCFG[bxi].BX, BY = GCFG[bxi].BY;
  dim3 T(BX, BY, 1);
  dim3 G((N_e + BX - 1)/BX, (nlev + BY - 1)/BY, 1);
  switch (V*100 + bxi) {
#define CASE_V(V_)                                                               \
    case V_*100+0: gpu_unblocked<V_, 32, 4><<<G,T>>>(ARGS); break;               \
    case V_*100+1: gpu_unblocked<V_, 32, 8><<<G,T>>>(ARGS); break;               \
    case V_*100+2: gpu_unblocked<V_, 64, 4><<<G,T>>>(ARGS); break;               \
    case V_*100+3: gpu_unblocked<V_, 16,16><<<G,T>>>(ARGS); break;               \
    case V_*100+4: gpu_unblocked<V_,128, 1><<<G,T>>>(ARGS); break;
    CASE_V(1) CASE_V(2) CASE_V(3) CASE_V(4)
#undef CASE_V
  }
}

static void launch_blocked(int bi, int bxi, double *d_w, const double *d_vie,
                           const double *d_vti, const double *d_gh, const double *d_iv,
                           const double *d_ft, const double *d_fn,
                           int N_e, int nlev, int jk0, int jk1) {
  int BX = GCFG[bxi].BX, BY = GCFG[bxi].BY;
  dim3 T(BX, BY, 1);
  dim3 G((N_e + BX - 1)/BX, (nlev + BY - 1)/BY, 1);
  switch (bi*100 + bxi) {
    case 0*100+0: gpu_blocked<8,  32, 4><<<G,T>>>(ARGS); break;
    case 0*100+1: gpu_blocked<8,  32, 8><<<G,T>>>(ARGS); break;
    case 0*100+2: gpu_blocked<8,  64, 4><<<G,T>>>(ARGS); break;
    case 0*100+3: gpu_blocked<8,  16,16><<<G,T>>>(ARGS); break;
    case 0*100+4: gpu_blocked<8, 128, 1><<<G,T>>>(ARGS); break;
    case 1*100+0: gpu_blocked<16, 32, 4><<<G,T>>>(ARGS); break;
    case 1*100+1: gpu_blocked<16, 32, 8><<<G,T>>>(ARGS); break;
    case 1*100+2: gpu_blocked<16, 64, 4><<<G,T>>>(ARGS); break;
    case 1*100+3: gpu_blocked<16, 16,16><<<G,T>>>(ARGS); break;
    case 1*100+4: gpu_blocked<16,128, 1><<<G,T>>>(ARGS); break;
    case 2*100+0: gpu_blocked<32, 32, 4><<<G,T>>>(ARGS); break;
    case 2*100+1: gpu_blocked<32, 32, 8><<<G,T>>>(ARGS); break;
    case 2*100+2: gpu_blocked<32, 64, 4><<<G,T>>>(ARGS); break;
    case 2*100+3: gpu_blocked<32, 16,16><<<G,T>>>(ARGS); break;
    case 2*100+4: gpu_blocked<32,128, 1><<<G,T>>>(ARGS); break;
    case 3*100+0: gpu_blocked<64, 32, 4><<<G,T>>>(ARGS); break;
    case 3*100+1: gpu_blocked<64, 32, 8><<<G,T>>>(ARGS); break;
    case 3*100+2: gpu_blocked<64, 64, 4><<<G,T>>>(ARGS); break;
    case 3*100+3: gpu_blocked<64, 16,16><<<G,T>>>(ARGS); break;
    case 3*100+4: gpu_blocked<64,128, 1><<<G,T>>>(ARGS); break;
    case 4*100+0: gpu_blocked<128,32, 4><<<G,T>>>(ARGS); break;
    case 4*100+1: gpu_blocked<128,32, 8><<<G,T>>>(ARGS); break;
    case 4*100+2: gpu_blocked<128,64, 4><<<G,T>>>(ARGS); break;
    case 4*100+3: gpu_blocked<128,16,16><<<G,T>>>(ARGS); break;
    case 4*100+4: gpu_blocked<128,128,1><<<G,T>>>(ARGS); break;
  }
}

template <int TX, int TY>
static void launch_tiled_inner(int bxi, dim3 G, dim3 T, double *d_w,
                               const double *d_vie, const double *d_vti,
                               const double *d_gh, const double *d_iv,
                               const double *d_ft, const double *d_fn,
                               int N_e, int nlev, int jk0, int jk1) {
  switch (bxi) {
    case 0: gpu_tiled<TX,TY, 32, 4><<<G,T>>>(ARGS); break;
    case 1: gpu_tiled<TX,TY, 32, 8><<<G,T>>>(ARGS); break;
    case 2: gpu_tiled<TX,TY, 64, 4><<<G,T>>>(ARGS); break;
    case 3: gpu_tiled<TX,TY, 16,16><<<G,T>>>(ARGS); break;
    case 4: gpu_tiled<TX,TY,128, 1><<<G,T>>>(ARGS); break;
  }
}
static void launch_tiled(int txi, int tyi, int bxi, double *d_w,
                         const double *d_vie, const double *d_vti,
                         const double *d_gh, const double *d_iv,
                         const double *d_ft, const double *d_fn,
                         int N_e, int nlev, int jk0, int jk1) {
  int BX = GCFG[bxi].BX, BY = GCFG[bxi].BY;
  dim3 T(BX, BY, 1);
  dim3 G((N_e + BX - 1)/BX, (nlev + BY - 1)/BY, 1);
#define T4(TX_, TY_) launch_tiled_inner<TX_, TY_>(bxi, G, T, d_w, d_vie, d_vti, d_gh, d_iv, d_ft, d_fn, N_e, nlev, jk0, jk1)
  switch (txi*4 + tyi) {
    case 0*4+0: T4(8,8);   break;  case 0*4+1: T4(8,16);  break;
    case 0*4+2: T4(8,32);  break;  case 0*4+3: T4(8,64);  break;
    case 1*4+0: T4(16,8);  break;  case 1*4+1: T4(16,16); break;
    case 1*4+2: T4(16,32); break;  case 1*4+3: T4(16,64); break;
    case 2*4+0: T4(32,8);  break;  case 2*4+1: T4(32,16); break;
    case 2*4+2: T4(32,32); break;  case 2*4+3: T4(32,64); break;
    case 3*4+0: T4(64,8);  break;  case 3*4+1: T4(64,16); break;
    case 3*4+2: T4(64,32); break;  case 3*4+3: T4(64,64); break;
  }
#undef T4
}

#undef ARGS
#define ARGS d_w,d_vie,d_vti,d_gh,d_iv,d_ft,d_fn

static void run_configs_unblocked(
    FILE *fcsv, int V, BenchData &bd, int nlev_end, const char *dist,
    double *d_w, const double *d_vie, const double *d_vti,
    const double *d_gh, const double *d_iv, const double *d_ft, const double *d_fn,
    hipEvent_t ev0, hipEvent_t ev1) {
  int jk0 = 0, jk1 = nlev_end;
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++)
      launch_unblocked(V, bxi, ARGS, bd.N_e, bd.nlev, jk0, jk1);
    HIP_CHECK(hipDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      HIP_CHECK(hipEventRecord(ev0, 0));
      launch_unblocked(V, bxi, ARGS, bd.N_e, bd.nlev, jk0, jk1);
      HIP_CHECK(hipEventRecord(ev1, 0));
      HIP_CHECK(hipEventSynchronize(ev1));
      float ms; HIP_CHECK(hipEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,%d,%d,%d,%s,V%d_%s,run%d,%d,%.6f\n",
              V, bd.nlev, bd.N_e, dist, V, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_blocked(
    FILE *fcsv, int bi, BenchData &bd, int nlev_end, const char *dist,
    double *d_w, const double *d_vie, const double *d_vti,
    const double *d_gh, const double *d_iv, const double *d_ft, const double *d_fn,
    hipEvent_t ev0, hipEvent_t ev1) {
  int B = BLOCK_SIZES[bi];
  int jk0 = 0, jk1 = nlev_end;
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++)
      launch_blocked(bi, bxi, ARGS, bd.N_e, bd.nlev, jk0, jk1);
    HIP_CHECK(hipDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      HIP_CHECK(hipEventRecord(ev0, 0));
      launch_blocked(bi, bxi, ARGS, bd.N_e, bd.nlev, jk0, jk1);
      HIP_CHECK(hipEventRecord(ev1, 0));
      HIP_CHECK(hipEventSynchronize(ev1));
      float ms; HIP_CHECK(hipEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,0,%d,%d,%s,B%d_%s,run%d,%d,%.6f\n",
              bd.nlev, bd.N_e, dist, B, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_tiled(
    FILE *fcsv, int txi, int tyi, BenchData &bd, int nlev_end, const char *dist,
    double *d_w, const double *d_vie, const double *d_vti,
    const double *d_gh, const double *d_iv, const double *d_ft, const double *d_fn,
    hipEvent_t ev0, hipEvent_t ev1) {
  int TX = TILE_X_VALUES[txi], TY = TILE_Y_VALUES[tyi + 1];
  int jk0 = 0, jk1 = nlev_end;
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++)
      launch_tiled(txi, tyi, bxi, ARGS, bd.N_e, bd.nlev, jk0, jk1);
    HIP_CHECK(hipDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      HIP_CHECK(hipEventRecord(ev0, 0));
      launch_tiled(txi, tyi, bxi, ARGS, bd.N_e, bd.nlev, jk0, jk1);
      HIP_CHECK(hipEventRecord(ev1, 0));
      HIP_CHECK(hipEventSynchronize(ev1));
      float ms; HIP_CHECK(hipEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,0,%d,%d,%s,t%02dx%02d_%s,run%d,%d,%.6f\n",
              bd.nlev, bd.N_e, dist, TX, TY, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}

int main(int argc, char *argv[]) {
  const char *csv = (argc >= 2) ? argv[1] : "z_v_grad_w_full_gpu.csv";
  FILE *fcsv = fopen(csv, "w"); if (!fcsv) { perror("fopen"); return 1; }
  fprintf(fcsv, "backend,V,nlev,N_e,cell_dist,config_label,run_label,run_id,time_ms\n");

  int step = (argc >= 3) ? atoi(argv[2]) : 9;
  std::string gp = icon_global_path(step), pp = icon_patch_path(step);
  int icon_nproma = icon_read_nproma(gp.c_str());
  IconEdgeData ied;
  bool have_exact = (icon_nproma > 0) && icon_load_patch(pp.c_str(), icon_nproma, ied);
  int N_e = have_exact ? ied.n_edges : 122880;

  double *d_w,*d_vie,*d_vti,*d_gh,*d_iv,*d_ft,*d_fn;
  size_t sz = (size_t)N_e * NLEVS[0];
  HIP_CHECK(hipMalloc(&d_w,   sz*8)); HIP_CHECK(hipMalloc(&d_vie, sz*8));
  HIP_CHECK(hipMalloc(&d_vti, sz*8));
  HIP_CHECK(hipMalloc(&d_gh, NLEVS[0]*8)); HIP_CHECK(hipMalloc(&d_iv, NLEVS[0]*8));
  HIP_CHECK(hipMalloc(&d_ft, N_e*8));      HIP_CHECK(hipMalloc(&d_fn, N_e*8));
  hipEvent_t ev0, ev1; hipEventCreate(&ev0); hipEventCreate(&ev1);

  const char *dists[3] = {"uniform", "normal_var1", "exact"};
  int ndists = have_exact ? 3 : 2;

  for (int ni = 0; ni < N_NLEVS; ni++) {
    int nlev = NLEVS[ni], nlev_end = nlev;

    for (int di = 0; di < ndists; di++) {
      for (int V = 1; V <= 4; V++) {
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant(V);
        HIP_CHECK(hipMemcpy(d_w,   bd.h_w,   sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_vie, bd.h_vie, sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_vti, bd.h_vti, sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_gh, bd.h_gradh, nlev*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_iv, bd.h_invr,  nlev*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_ft, bd.h_ft, N_e*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_fn, bd.h_fn, N_e*8, hipMemcpyHostToDevice));
        run_configs_unblocked(fcsv, V, bd, nlev_end, dists[di],
                              d_w, d_vie, d_vti, d_gh, d_iv, d_ft, d_fn, ev0, ev1);
        fflush(fcsv); bd.free_all();
      }
    }

    for (int di = 0; di < ndists; di++) {
      for (int bi = 0; bi < N_BLOCK_SIZES; bi++) {
        int B = BLOCK_SIZES[bi];
        if (N_e % B != 0) { printf("SKIP GPU B=%d\n", B); continue; }
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant_blocked(B);
        HIP_CHECK(hipMemcpy(d_w,   bd.h_w,   sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_vie, bd.h_vie, sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_vti, bd.h_vti, sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_gh, bd.h_gradh, nlev*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_iv, bd.h_invr,  nlev*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_ft, bd.h_ft, N_e*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_fn, bd.h_fn, N_e*8, hipMemcpyHostToDevice));
        run_configs_blocked(fcsv, bi, bd, nlev_end, dists[di],
                            d_w, d_vie, d_vti, d_gh, d_iv, d_ft, d_fn, ev0, ev1);
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
          HIP_CHECK(hipMemcpy(d_w,   bd.h_w,   sz*8, hipMemcpyHostToDevice));
          HIP_CHECK(hipMemcpy(d_vie, bd.h_vie, sz*8, hipMemcpyHostToDevice));
          HIP_CHECK(hipMemcpy(d_vti, bd.h_vti, sz*8, hipMemcpyHostToDevice));
          HIP_CHECK(hipMemcpy(d_gh, bd.h_gradh, nlev*8, hipMemcpyHostToDevice));
          HIP_CHECK(hipMemcpy(d_iv, bd.h_invr,  nlev*8, hipMemcpyHostToDevice));
          HIP_CHECK(hipMemcpy(d_ft, bd.h_ft, N_e*8, hipMemcpyHostToDevice));
          HIP_CHECK(hipMemcpy(d_fn, bd.h_fn, N_e*8, hipMemcpyHostToDevice));
          run_configs_tiled(fcsv, txi, tyi, bd, nlev_end, dists[di],
                            d_w, d_vie, d_vti, d_gh, d_iv, d_ft, d_fn, ev0, ev1);
          fflush(fcsv); bd.free_all();
        }
      }
    }
  }

  hipFree(d_w); hipFree(d_vie); hipFree(d_vti);
  hipFree(d_gh); hipFree(d_iv); hipFree(d_ft); hipFree(d_fn);
  hipEventDestroy(ev0); hipEventDestroy(ev1);
  if (have_exact) ied.free_all();
  fclose(fcsv);
  return 0;
}
