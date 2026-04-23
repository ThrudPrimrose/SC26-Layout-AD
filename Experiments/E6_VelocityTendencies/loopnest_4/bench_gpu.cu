/*
 * bench_gpu.cu -- loopnest_4 (Nest 24) GPU (CUDA) benchmark.
 * Indirect stencil + CFL-gated diffusion, partial vertical.
 */
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "bench_common.h"
#include "../loopnest_1/icon_data_loader.h"

#define CUDA_CHECK(x) do { cudaError_t e=(x); \
  if(e!=cudaSuccess){fprintf(stderr,"CUDA err %s:%d %s\n",__FILE__,__LINE__,cudaGetErrorString(e));std::abort();} } while(0)

static constexpr int GPU_WARMUP = 5;
static constexpr int GPU_NRUNS  = 100;

#define KARGS_DEV \
  double *__restrict__ ddt, const double *__restrict__ vn, const double *__restrict__ zw, \
  const double *__restrict__ zeta, const double *__restrict__ ddqz, \
  const double *__restrict__ cle, const double *__restrict__ gf, \
  const double *__restrict__ area, const double *__restrict__ tang, const double *__restrict__ invp, \
  const int *__restrict__ ici, const int *__restrict__ iqi, const int *__restrict__ ivi, \
  const int *__restrict__ lvlm, \
  int N_e, int nlev, int jk0, int jk1

template <int V, int BX, int BY>
__global__ void gpu_unblocked(KARGS_DEV) {
  int je = blockIdx.x*BX + threadIdx.x;
  int jk = blockIdx.y*BY + threadIdx.y;
  if (je >= N_e || jk < jk0 || jk >= jk1) return;
  if (!(lvlm[jk] || lvlm[jk+1])) return;
  int c  = IC<V>(je, jk, N_e, nlev);
  int c0 = IC<V>(ici[je*2+0], jk, N_e, nlev);
  int c1 = IC<V>(ici[je*2+1], jk, N_e, nlev);
  double wce = cle[je*2+0]*zw[c0] + cle[je*2+1]*zw[c1];
  double dq  = ddqz[c];
  if (fabs(wce) > CFL_W_LIMIT * dq) {
    double dif = SFEX * fmin(0.85 - CFL_W_LIMIT*DTIME,
                             fabs(wce)*DTIME/dq - CFL_W_LIMIT*DTIME);
    double s = gf[je*5+0]*vn[c];
    s += gf[je*5+1]*vn[IC<V>(iqi[je*4+0], jk, N_e, nlev)];
    s += gf[je*5+2]*vn[IC<V>(iqi[je*4+1], jk, N_e, nlev)];
    s += gf[je*5+3]*vn[IC<V>(iqi[je*4+2], jk, N_e, nlev)];
    s += gf[je*5+4]*vn[IC<V>(iqi[je*4+3], jk, N_e, nlev)];
    s += tang[je]*invp[je]*(zeta[IC<V>(ivi[je*2+1], jk, N_e, nlev)]
                           - zeta[IC<V>(ivi[je*2+0], jk, N_e, nlev)]);
    ddt[c] += dif * area[je] * s;
  }
}

template <int B, int BX, int BY>
__global__ void gpu_blocked(KARGS_DEV) {
  int je = blockIdx.x*BX + threadIdx.x;
  int jk = blockIdx.y*BY + threadIdx.y;
  if (je >= N_e || jk < jk0 || jk >= jk1) return;
  if (!(lvlm[jk] || lvlm[jk+1])) return;
  int c  = IC_blocked(je, jk, B, nlev);
  int c0 = IC_blocked(ici[je*2+0], jk, B, nlev);
  int c1 = IC_blocked(ici[je*2+1], jk, B, nlev);
  double wce = cle[je*2+0]*zw[c0] + cle[je*2+1]*zw[c1];
  double dq  = ddqz[c];
  if (fabs(wce) > CFL_W_LIMIT * dq) {
    double dif = SFEX * fmin(0.85 - CFL_W_LIMIT*DTIME,
                             fabs(wce)*DTIME/dq - CFL_W_LIMIT*DTIME);
    double s = gf[je*5+0]*vn[c];
    s += gf[je*5+1]*vn[IC_blocked(iqi[je*4+0], jk, B, nlev)];
    s += gf[je*5+2]*vn[IC_blocked(iqi[je*4+1], jk, B, nlev)];
    s += gf[je*5+3]*vn[IC_blocked(iqi[je*4+2], jk, B, nlev)];
    s += gf[je*5+4]*vn[IC_blocked(iqi[je*4+3], jk, B, nlev)];
    s += tang[je]*invp[je]*(zeta[IC_blocked(ivi[je*2+1], jk, B, nlev)]
                           - zeta[IC_blocked(ivi[je*2+0], jk, B, nlev)]);
    ddt[c] += dif * area[je] * s;
  }
}

template <int TX, int TY, int BX, int BY>
__global__ void gpu_tiled(KARGS_DEV) {
  int je = blockIdx.x*BX + threadIdx.x;
  int jk = blockIdx.y*BY + threadIdx.y;
  if (je >= N_e || jk < jk0 || jk >= jk1) return;
  if (!(lvlm[jk] || lvlm[jk+1])) return;
  int c  = IC_tiled(je, jk, TX, TY, N_e, nlev);
  int c0 = IC_tiled(ici[je*2+0], jk, TX, TY, N_e, nlev);
  int c1 = IC_tiled(ici[je*2+1], jk, TX, TY, N_e, nlev);
  double wce = cle[je*2+0]*zw[c0] + cle[je*2+1]*zw[c1];
  double dq  = ddqz[c];
  if (fabs(wce) > CFL_W_LIMIT * dq) {
    double dif = SFEX * fmin(0.85 - CFL_W_LIMIT*DTIME,
                             fabs(wce)*DTIME/dq - CFL_W_LIMIT*DTIME);
    double s = gf[je*5+0]*vn[c];
    s += gf[je*5+1]*vn[IC_tiled(iqi[je*4+0], jk, TX, TY, N_e, nlev)];
    s += gf[je*5+2]*vn[IC_tiled(iqi[je*4+1], jk, TX, TY, N_e, nlev)];
    s += gf[je*5+3]*vn[IC_tiled(iqi[je*4+2], jk, TX, TY, N_e, nlev)];
    s += gf[je*5+4]*vn[IC_tiled(iqi[je*4+3], jk, TX, TY, N_e, nlev)];
    s += tang[je]*invp[je]*(zeta[IC_tiled(ivi[je*2+1], jk, TX, TY, N_e, nlev)]
                           - zeta[IC_tiled(ivi[je*2+0], jk, TX, TY, N_e, nlev)]);
    ddt[c] += dif * area[je] * s;
  }
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

#define ARGS d_ddt,d_vn,d_zw,d_ze,d_dq,d_cle,d_gf,d_ar,d_ta,d_ip, \
             d_ici,d_iqi,d_ivi,d_lvm,N_e,nlev,jk0,jk1

static void launch_unblocked(int V, int bxi, double *d_ddt,
                             const double *d_vn, const double *d_zw, const double *d_ze,
                             const double *d_dq, const double *d_cle, const double *d_gf,
                             const double *d_ar, const double *d_ta, const double *d_ip,
                             const int *d_ici, const int *d_iqi, const int *d_ivi,
                             const int *d_lvm, int N_e, int nlev, int jk0, int jk1) {
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

static void launch_blocked(int bi, int bxi, double *d_ddt,
                           const double *d_vn, const double *d_zw, const double *d_ze,
                           const double *d_dq, const double *d_cle, const double *d_gf,
                           const double *d_ar, const double *d_ta, const double *d_ip,
                           const int *d_ici, const int *d_iqi, const int *d_ivi,
                           const int *d_lvm, int N_e, int nlev, int jk0, int jk1) {
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
static void launch_tiled_inner(int bxi, dim3 G, dim3 T, double *d_ddt,
                               const double *d_vn, const double *d_zw, const double *d_ze,
                               const double *d_dq, const double *d_cle, const double *d_gf,
                               const double *d_ar, const double *d_ta, const double *d_ip,
                               const int *d_ici, const int *d_iqi, const int *d_ivi,
                               const int *d_lvm, int N_e, int nlev, int jk0, int jk1) {
  switch (bxi) {
    case 0: gpu_tiled<TX,TY, 32, 4><<<G,T>>>(ARGS); break;
    case 1: gpu_tiled<TX,TY, 32, 8><<<G,T>>>(ARGS); break;
    case 2: gpu_tiled<TX,TY, 64, 4><<<G,T>>>(ARGS); break;
    case 3: gpu_tiled<TX,TY, 16,16><<<G,T>>>(ARGS); break;
    case 4: gpu_tiled<TX,TY,128, 1><<<G,T>>>(ARGS); break;
  }
}
static void launch_tiled(int txi, int tyi, int bxi, double *d_ddt,
                         const double *d_vn, const double *d_zw, const double *d_ze,
                         const double *d_dq, const double *d_cle, const double *d_gf,
                         const double *d_ar, const double *d_ta, const double *d_ip,
                         const int *d_ici, const int *d_iqi, const int *d_ivi,
                         const int *d_lvm, int N_e, int nlev, int jk0, int jk1) {
  int BX = GCFG[bxi].BX, BY = GCFG[bxi].BY;
  dim3 T(BX, BY, 1);
  dim3 G((N_e + BX - 1)/BX, (nlev + BY - 1)/BY, 1);
#define T4(TX_, TY_) launch_tiled_inner<TX_, TY_>(bxi, G, T, d_ddt, d_vn, d_zw, d_ze, \
                                                   d_dq, d_cle, d_gf, d_ar, d_ta, d_ip, \
                                                   d_ici, d_iqi, d_ivi, d_lvm, N_e, nlev, jk0, jk1)
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
#define ARGS d_ddt,d_vn,d_zw,d_ze,d_dq,d_cle,d_gf,d_ar,d_ta,d_ip,d_ici,d_iqi,d_ivi,d_lvm

static void run_configs_unblocked(
    FILE *fcsv, int V, BenchData &bd, int jk0, int jk1, const char *dist,
    double *d_ddt, const double *d_vn, const double *d_zw, const double *d_ze,
    const double *d_dq, const double *d_cle, const double *d_gf,
    const double *d_ar, const double *d_ta, const double *d_ip,
    const int *d_ici, const int *d_iqi, const int *d_ivi, const int *d_lvm,
    cudaEvent_t ev0, cudaEvent_t ev1) {
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++)
      launch_unblocked(V, bxi, ARGS, bd.N_e, bd.nlev, jk0, jk1);
    CUDA_CHECK(cudaDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      CUDA_CHECK(cudaEventRecord(ev0));
      launch_unblocked(V, bxi, ARGS, bd.N_e, bd.nlev, jk0, jk1);
      CUDA_CHECK(cudaEventRecord(ev1));
      CUDA_CHECK(cudaEventSynchronize(ev1));
      float ms; CUDA_CHECK(cudaEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,%d,%d,%d,%s,V%d_%s,run%d,%d,%.6f\n",
              V, bd.nlev, bd.N_e, dist, V, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_blocked(
    FILE *fcsv, int bi, BenchData &bd, int jk0, int jk1, const char *dist,
    double *d_ddt, const double *d_vn, const double *d_zw, const double *d_ze,
    const double *d_dq, const double *d_cle, const double *d_gf,
    const double *d_ar, const double *d_ta, const double *d_ip,
    const int *d_ici, const int *d_iqi, const int *d_ivi, const int *d_lvm,
    cudaEvent_t ev0, cudaEvent_t ev1) {
  int B = BLOCK_SIZES[bi];
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++)
      launch_blocked(bi, bxi, ARGS, bd.N_e, bd.nlev, jk0, jk1);
    CUDA_CHECK(cudaDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      CUDA_CHECK(cudaEventRecord(ev0));
      launch_blocked(bi, bxi, ARGS, bd.N_e, bd.nlev, jk0, jk1);
      CUDA_CHECK(cudaEventRecord(ev1));
      CUDA_CHECK(cudaEventSynchronize(ev1));
      float ms; CUDA_CHECK(cudaEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,0,%d,%d,%s,B%d_%s,run%d,%d,%.6f\n",
              bd.nlev, bd.N_e, dist, B, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_tiled(
    FILE *fcsv, int txi, int tyi, BenchData &bd, int jk0, int jk1, const char *dist,
    double *d_ddt, const double *d_vn, const double *d_zw, const double *d_ze,
    const double *d_dq, const double *d_cle, const double *d_gf,
    const double *d_ar, const double *d_ta, const double *d_ip,
    const int *d_ici, const int *d_iqi, const int *d_ivi, const int *d_lvm,
    cudaEvent_t ev0, cudaEvent_t ev1) {
  int TX = TILE_X_VALUES[txi], TY = TILE_Y_VALUES[tyi + 1];
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++)
      launch_tiled(txi, tyi, bxi, ARGS, bd.N_e, bd.nlev, jk0, jk1);
    CUDA_CHECK(cudaDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      CUDA_CHECK(cudaEventRecord(ev0));
      launch_tiled(txi, tyi, bxi, ARGS, bd.N_e, bd.nlev, jk0, jk1);
      CUDA_CHECK(cudaEventRecord(ev1));
      CUDA_CHECK(cudaEventSynchronize(ev1));
      float ms; CUDA_CHECK(cudaEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,0,%d,%d,%s,t%02dx%02d_%s,run%d,%d,%.6f\n",
              bd.nlev, bd.N_e, dist, TX, TY, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}

int main(int argc, char *argv[]) {
  const char *csv = (argc >= 2) ? argv[1] : "ddt_vn_vert_gpu.csv";
  FILE *fcsv = fopen(csv, "w"); if (!fcsv) { perror("fopen"); return 1; }
  fprintf(fcsv, "backend,V,nlev,N_e,cell_dist,config_label,run_label,run_id,time_ms\n");

  int step = (argc >= 3) ? atoi(argv[2]) : 9;
  std::string gp = icon_global_path(step), pp = icon_patch_path(step);
  int icon_nproma = icon_read_nproma(gp.c_str());
  IconEdgeData ied;
  bool have_exact = (icon_nproma > 0) && icon_load_patch(pp.c_str(), icon_nproma, ied);
  int N_e = have_exact ? ied.n_edges : 122880;

  double *d_ddt,*d_vn,*d_zw,*d_ze,*d_dq,*d_cle,*d_gf,*d_ar,*d_ta,*d_ip;
  int *d_ici,*d_iqi,*d_ivi,*d_lvm;
  int max_nlev = 0;
  for (int ni = 0; ni < N_NLEVS; ni++) if (NLEVS[ni] > max_nlev) max_nlev = NLEVS[ni];
  size_t sz_max = (size_t)N_e * max_nlev;
  CUDA_CHECK(cudaMalloc(&d_ddt, sz_max*8)); CUDA_CHECK(cudaMalloc(&d_vn, sz_max*8));
  CUDA_CHECK(cudaMalloc(&d_zw, sz_max*8));  CUDA_CHECK(cudaMalloc(&d_ze, sz_max*8));
  CUDA_CHECK(cudaMalloc(&d_dq, sz_max*8));
  CUDA_CHECK(cudaMalloc(&d_cle, (size_t)N_e*2*8));
  CUDA_CHECK(cudaMalloc(&d_gf,  (size_t)N_e*5*8));
  CUDA_CHECK(cudaMalloc(&d_ar, N_e*8));
  CUDA_CHECK(cudaMalloc(&d_ta, N_e*8));
  CUDA_CHECK(cudaMalloc(&d_ip, N_e*8));
  CUDA_CHECK(cudaMalloc(&d_ici, (size_t)N_e*2*sizeof(int)));
  CUDA_CHECK(cudaMalloc(&d_iqi, (size_t)N_e*4*sizeof(int)));
  CUDA_CHECK(cudaMalloc(&d_ivi, (size_t)N_e*2*sizeof(int)));
  CUDA_CHECK(cudaMalloc(&d_lvm, (size_t)(max_nlev+1)*sizeof(int)));

  cudaEvent_t ev0, ev1; cudaEventCreate(&ev0); cudaEventCreate(&ev1);

  const char *dists[3] = {"uniform", "normal_var1", "exact"};
  int ndists = have_exact ? 3 : 2;

  for (int ni = 0; ni < N_NLEVS; ni++) {
    int nlev = NLEVS[ni];
    int jk0 = jk_lo_for(nlev), jk1 = jk_hi_for(nlev);
    size_t sz = (size_t)N_e * nlev;

    for (int di = 0; di < ndists; di++) {
      for (int V = 1; V <= 4; V++) {
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant(V);
        CUDA_CHECK(cudaMemcpy(d_ddt, bd.h_ddt, sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_vn,  bd.h_vn,  sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_zw,  bd.h_zw,  sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_ze,  bd.h_zeta,sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_dq,  bd.h_ddqz,sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_cle, bd.h_cle, (size_t)N_e*2*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_gf,  bd.h_gf,  (size_t)N_e*5*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_ar,  bd.h_area,N_e*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_ta,  bd.h_tang,N_e*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_ip,  bd.h_invp,N_e*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_ici, bd.h_ici, (size_t)N_e*2*sizeof(int), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_iqi, bd.h_iqi, (size_t)N_e*4*sizeof(int), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_ivi, bd.h_ivi, (size_t)N_e*2*sizeof(int), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_lvm, bd.h_lvlm,(nlev+1)*sizeof(int), cudaMemcpyHostToDevice));
        run_configs_unblocked(fcsv, V, bd, jk0, jk1, dists[di],
                              d_ddt, d_vn, d_zw, d_ze, d_dq, d_cle, d_gf, d_ar, d_ta, d_ip,
                              d_ici, d_iqi, d_ivi, d_lvm, ev0, ev1);
        fflush(fcsv); bd.free_all();
      }
    }

    for (int di = 0; di < ndists; di++) {
      for (int bi = 0; bi < N_BLOCK_SIZES; bi++) {
        int B = BLOCK_SIZES[bi];
        if (N_e % B != 0) { printf("SKIP GPU B=%d\n", B); continue; }
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant_blocked(B);
        CUDA_CHECK(cudaMemcpy(d_ddt, bd.h_ddt, sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_vn,  bd.h_vn,  sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_zw,  bd.h_zw,  sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_ze,  bd.h_zeta,sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_dq,  bd.h_ddqz,sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_cle, bd.h_cle, (size_t)N_e*2*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_gf,  bd.h_gf,  (size_t)N_e*5*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_ar,  bd.h_area,N_e*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_ta,  bd.h_tang,N_e*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_ip,  bd.h_invp,N_e*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_ici, bd.h_ici, (size_t)N_e*2*sizeof(int), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_iqi, bd.h_iqi, (size_t)N_e*4*sizeof(int), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_ivi, bd.h_ivi, (size_t)N_e*2*sizeof(int), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_lvm, bd.h_lvlm,(nlev+1)*sizeof(int), cudaMemcpyHostToDevice));
        run_configs_blocked(fcsv, bi, bd, jk0, jk1, dists[di],
                            d_ddt, d_vn, d_zw, d_ze, d_dq, d_cle, d_gf, d_ar, d_ta, d_ip,
                            d_ici, d_iqi, d_ivi, d_lvm, ev0, ev1);
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
          CUDA_CHECK(cudaMemcpy(d_ddt, bd.h_ddt, sz*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_vn,  bd.h_vn,  sz*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_zw,  bd.h_zw,  sz*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_ze,  bd.h_zeta,sz*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_dq,  bd.h_ddqz,sz*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_cle, bd.h_cle, (size_t)N_e*2*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_gf,  bd.h_gf,  (size_t)N_e*5*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_ar,  bd.h_area,N_e*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_ta,  bd.h_tang,N_e*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_ip,  bd.h_invp,N_e*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_ici, bd.h_ici, (size_t)N_e*2*sizeof(int), cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_iqi, bd.h_iqi, (size_t)N_e*4*sizeof(int), cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_ivi, bd.h_ivi, (size_t)N_e*2*sizeof(int), cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_lvm, bd.h_lvlm,(nlev+1)*sizeof(int), cudaMemcpyHostToDevice));
          run_configs_tiled(fcsv, txi, tyi, bd, jk0, jk1, dists[di],
                            d_ddt, d_vn, d_zw, d_ze, d_dq, d_cle, d_gf, d_ar, d_ta, d_ip,
                            d_ici, d_iqi, d_ivi, d_lvm, ev0, ev1);
          fflush(fcsv); bd.free_all();
        }
      }
    }
  }

  cudaFree(d_ddt); cudaFree(d_vn); cudaFree(d_zw); cudaFree(d_ze); cudaFree(d_dq);
  cudaFree(d_cle); cudaFree(d_gf); cudaFree(d_ar); cudaFree(d_ta); cudaFree(d_ip);
  cudaFree(d_ici); cudaFree(d_iqi); cudaFree(d_ivi); cudaFree(d_lvm);
  cudaEventDestroy(ev0); cudaEventDestroy(ev1);
  if (have_exact) ied.free_all();
  fclose(fcsv);
  return 0;
}
