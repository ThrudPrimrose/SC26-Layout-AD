/*
 * bench_gpu_hip.cpp -- loopnest_5 (Nest 2) GPU (HIP) benchmark.
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
  double *__restrict__ vnie, double *__restrict__ zvt, double *__restrict__ zk, \
  const double *__restrict__ vn, const double *__restrict__ vt, \
  const double *__restrict__ ubc, const double *__restrict__ wgt, \
  int N_e, int nlev, int nlevp1

template <int V, int BX>
__global__ void gpu_unblocked(KARGS_DEV) {
  int je = blockIdx.x*BX + threadIdx.x;
  if (je >= N_e) return;
  int ct  = IC<V>(je, 0,        N_e, nlevp1);
  int cb  = IC<V>(je, nlevp1-1, N_e, nlevp1);
  int cm1 = IC<V>(je, nlevp1-2, N_e, nlevp1);
  int cm2 = IC<V>(je, nlevp1-3, N_e, nlevp1);
  int cm3 = IC<V>(je, nlevp1-4, N_e, nlevp1);
  double u0 = ubc[je*2+0], u1 = ubc[je*2+1];
  vnie[ct] = u0 + DT_LINIT_UBC * u1;
  double vtv = vt[ct]; zvt[ct] = vtv;
  double vnv = vn[ct]; zk[ct]  = 0.5 * (vnv*vnv + vtv*vtv);
  vnie[cb] = wgt[je*3+0]*vn[cm1] + wgt[je*3+1]*vn[cm2] + wgt[je*3+2]*vn[cm3];
}

template <int B, int BX>
__global__ void gpu_blocked(KARGS_DEV) {
  int je = blockIdx.x*BX + threadIdx.x;
  if (je >= N_e) return;
  int ct  = IC_blocked(je, 0,        B, nlevp1);
  int cb  = IC_blocked(je, nlevp1-1, B, nlevp1);
  int cm1 = IC_blocked(je, nlevp1-2, B, nlevp1);
  int cm2 = IC_blocked(je, nlevp1-3, B, nlevp1);
  int cm3 = IC_blocked(je, nlevp1-4, B, nlevp1);
  double u0 = ubc[je*2+0], u1 = ubc[je*2+1];
  vnie[ct] = u0 + DT_LINIT_UBC * u1;
  double vtv = vt[ct]; zvt[ct] = vtv;
  double vnv = vn[ct]; zk[ct]  = 0.5 * (vnv*vnv + vtv*vtv);
  vnie[cb] = wgt[je*3+0]*vn[cm1] + wgt[je*3+1]*vn[cm2] + wgt[je*3+2]*vn[cm3];
}

struct GCfg { int BX; const char *tag; };
static const GCfg GCFG[] = {
  { 64,  "bx64"}, {128, "bx128"}, {256, "bx256"}, {512, "bx512"}, {1024, "bx1024"}
};
static constexpr int N_GCFG = sizeof(GCFG)/sizeof(GCFG[0]);

#define ARGS d_vnie,d_zvt,d_zk,d_vn,d_vt,d_ubc,d_wgt,N_e,nlev,nlevp1

static void launch_unblocked(int V, int bxi, double *d_vnie, double *d_zvt, double *d_zk,
                             const double *d_vn, const double *d_vt,
                             const double *d_ubc, const double *d_wgt,
                             int N_e, int nlev, int nlevp1) {
  int BX = GCFG[bxi].BX;
  dim3 T(BX, 1, 1);
  dim3 G((N_e + BX - 1)/BX, 1, 1);
  switch (V*100 + bxi) {
#define CASE_V(V_) \
    case V_*100+0: gpu_unblocked<V_, 64  ><<<G,T>>>(ARGS); break; \
    case V_*100+1: gpu_unblocked<V_, 128 ><<<G,T>>>(ARGS); break; \
    case V_*100+2: gpu_unblocked<V_, 256 ><<<G,T>>>(ARGS); break; \
    case V_*100+3: gpu_unblocked<V_, 512 ><<<G,T>>>(ARGS); break; \
    case V_*100+4: gpu_unblocked<V_, 1024><<<G,T>>>(ARGS); break;
    CASE_V(1) CASE_V(2) CASE_V(3) CASE_V(4)
#undef CASE_V
  }
}

static void launch_blocked(int bi, int bxi, double *d_vnie, double *d_zvt, double *d_zk,
                           const double *d_vn, const double *d_vt,
                           const double *d_ubc, const double *d_wgt,
                           int N_e, int nlev, int nlevp1) {
  int BX = GCFG[bxi].BX;
  dim3 T(BX, 1, 1);
  dim3 G((N_e + BX - 1)/BX, 1, 1);
  switch (bi*100 + bxi) {
    case 0*100+0: gpu_blocked<8,   64  ><<<G,T>>>(ARGS); break;
    case 0*100+1: gpu_blocked<8,   128 ><<<G,T>>>(ARGS); break;
    case 0*100+2: gpu_blocked<8,   256 ><<<G,T>>>(ARGS); break;
    case 0*100+3: gpu_blocked<8,   512 ><<<G,T>>>(ARGS); break;
    case 0*100+4: gpu_blocked<8,   1024><<<G,T>>>(ARGS); break;
    case 1*100+0: gpu_blocked<16,  64  ><<<G,T>>>(ARGS); break;
    case 1*100+1: gpu_blocked<16,  128 ><<<G,T>>>(ARGS); break;
    case 1*100+2: gpu_blocked<16,  256 ><<<G,T>>>(ARGS); break;
    case 1*100+3: gpu_blocked<16,  512 ><<<G,T>>>(ARGS); break;
    case 1*100+4: gpu_blocked<16,  1024><<<G,T>>>(ARGS); break;
    case 2*100+0: gpu_blocked<32,  64  ><<<G,T>>>(ARGS); break;
    case 2*100+1: gpu_blocked<32,  128 ><<<G,T>>>(ARGS); break;
    case 2*100+2: gpu_blocked<32,  256 ><<<G,T>>>(ARGS); break;
    case 2*100+3: gpu_blocked<32,  512 ><<<G,T>>>(ARGS); break;
    case 2*100+4: gpu_blocked<32,  1024><<<G,T>>>(ARGS); break;
    case 3*100+0: gpu_blocked<64,  64  ><<<G,T>>>(ARGS); break;
    case 3*100+1: gpu_blocked<64,  128 ><<<G,T>>>(ARGS); break;
    case 3*100+2: gpu_blocked<64,  256 ><<<G,T>>>(ARGS); break;
    case 3*100+3: gpu_blocked<64,  512 ><<<G,T>>>(ARGS); break;
    case 3*100+4: gpu_blocked<64,  1024><<<G,T>>>(ARGS); break;
    case 4*100+0: gpu_blocked<128, 64  ><<<G,T>>>(ARGS); break;
    case 4*100+1: gpu_blocked<128, 128 ><<<G,T>>>(ARGS); break;
    case 4*100+2: gpu_blocked<128, 256 ><<<G,T>>>(ARGS); break;
    case 4*100+3: gpu_blocked<128, 512 ><<<G,T>>>(ARGS); break;
    case 4*100+4: gpu_blocked<128, 1024><<<G,T>>>(ARGS); break;
  }
}

#undef ARGS
#define ARGS d_vnie,d_zvt,d_zk,d_vn,d_vt,d_ubc,d_wgt

static void run_configs_unblocked(FILE *fcsv, int V, BenchData &bd, const char *dist,
    double *d_vnie, double *d_zvt, double *d_zk, const double *d_vn, const double *d_vt,
    const double *d_ubc, const double *d_wgt, hipEvent_t ev0, hipEvent_t ev1) {
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++)
      launch_unblocked(V, bxi, ARGS, bd.N_e, bd.nlev, bd.nlevp1);
    HIP_CHECK(hipDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      HIP_CHECK(hipEventRecord(ev0, 0));
      launch_unblocked(V, bxi, ARGS, bd.N_e, bd.nlev, bd.nlevp1);
      HIP_CHECK(hipEventRecord(ev1, 0));
      HIP_CHECK(hipEventSynchronize(ev1));
      float ms; HIP_CHECK(hipEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,%d,%d,%d,%s,V%d_%s,run%d,%d,%.6f\n",
              V, bd.nlev, bd.N_e, dist, V, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_blocked(FILE *fcsv, int bi, BenchData &bd, const char *dist,
    double *d_vnie, double *d_zvt, double *d_zk, const double *d_vn, const double *d_vt,
    const double *d_ubc, const double *d_wgt, hipEvent_t ev0, hipEvent_t ev1) {
  int B = BLOCK_SIZES[bi];
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++)
      launch_blocked(bi, bxi, ARGS, bd.N_e, bd.nlev, bd.nlevp1);
    HIP_CHECK(hipDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      HIP_CHECK(hipEventRecord(ev0, 0));
      launch_blocked(bi, bxi, ARGS, bd.N_e, bd.nlev, bd.nlevp1);
      HIP_CHECK(hipEventRecord(ev1, 0));
      HIP_CHECK(hipEventSynchronize(ev1));
      float ms; HIP_CHECK(hipEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,0,%d,%d,%s,B%d_%s,run%d,%d,%.6f\n",
              bd.nlev, bd.N_e, dist, B, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}

int main(int argc, char *argv[]) {
  const char *csv = (argc >= 2) ? argv[1] : "vn_ie_boundary_gpu.csv";
  FILE *fcsv = fopen(csv, "w"); if (!fcsv) { perror("fopen"); return 1; }
  fprintf(fcsv, "backend,V,nlev,N_e,cell_dist,config_label,run_label,run_id,time_ms\n");

  int step = (argc >= 3) ? atoi(argv[2]) : 9;
  std::string gp = icon_global_path(step), pp = icon_patch_path(step);
  int icon_nproma = icon_read_nproma(gp.c_str());
  IconEdgeData ied;
  bool have_exact = (icon_nproma > 0) && icon_load_patch(pp.c_str(), icon_nproma, ied);
  int N_e = have_exact ? ied.n_edges : 122880;

  double *d_vnie,*d_zvt,*d_zk,*d_vn,*d_vt,*d_ubc,*d_wgt;
  size_t sz = (size_t)N_e * (NLEVS[0] + 1);
  HIP_CHECK(hipMalloc(&d_vnie, sz*8));
  HIP_CHECK(hipMalloc(&d_zvt,  sz*8));
  HIP_CHECK(hipMalloc(&d_zk,   sz*8));
  HIP_CHECK(hipMalloc(&d_vn,   sz*8));
  HIP_CHECK(hipMalloc(&d_vt,   sz*8));
  HIP_CHECK(hipMalloc(&d_ubc, (size_t)N_e*2*8));
  HIP_CHECK(hipMalloc(&d_wgt, (size_t)N_e*3*8));
  hipEvent_t ev0, ev1; hipEventCreate(&ev0); hipEventCreate(&ev1);

  const char *dists[3] = {"uniform", "normal_var1", "exact"};
  int ndists = have_exact ? 3 : 2;

  for (int ni = 0; ni < N_NLEVS; ni++) {
    int nlev = NLEVS[ni];

    for (int di = 0; di < ndists; di++) {
      for (int V = 1; V <= 4; V++) {
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant(V);
        HIP_CHECK(hipMemcpy(d_vnie, bd.h_vnie, sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_zvt,  bd.h_zvt,  sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_zk,   bd.h_zk,   sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_vn,   bd.h_vn,   sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_vt,   bd.h_vt,   sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_ubc,  bd.h_ubc, (size_t)N_e*2*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_wgt,  bd.h_wgt, (size_t)N_e*3*8, hipMemcpyHostToDevice));
        run_configs_unblocked(fcsv, V, bd, dists[di],
                              d_vnie, d_zvt, d_zk, d_vn, d_vt, d_ubc, d_wgt, ev0, ev1);
        fflush(fcsv); bd.free_all();
      }
    }

    for (int di = 0; di < ndists; di++) {
      for (int bi = 0; bi < N_BLOCK_SIZES; bi++) {
        int B = BLOCK_SIZES[bi];
        if (N_e % B != 0) { printf("SKIP GPU B=%d\n", B); continue; }
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant_blocked(B);
        HIP_CHECK(hipMemcpy(d_vnie, bd.h_vnie, sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_zvt,  bd.h_zvt,  sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_zk,   bd.h_zk,   sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_vn,   bd.h_vn,   sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_vt,   bd.h_vt,   sz*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_ubc,  bd.h_ubc, (size_t)N_e*2*8, hipMemcpyHostToDevice));
        HIP_CHECK(hipMemcpy(d_wgt,  bd.h_wgt, (size_t)N_e*3*8, hipMemcpyHostToDevice));
        run_configs_blocked(fcsv, bi, bd, dists[di],
                            d_vnie, d_zvt, d_zk, d_vn, d_vt, d_ubc, d_wgt, ev0, ev1);
        fflush(fcsv); bd.free_all();
      }
    }
  }

  hipFree(d_vnie); hipFree(d_zvt); hipFree(d_zk);
  hipFree(d_vn);   hipFree(d_vt);  hipFree(d_ubc); hipFree(d_wgt);
  hipEventDestroy(ev0); hipEventDestroy(ev1);
  if (have_exact) ied.free_all();
  fclose(fcsv);
  return 0;
}
