/*
 * bench_gpu.cu -- loopnest_2 (Nest 5) GPU (CUDA) benchmark.
 * Direct stencil: z_w_concorr_me = vn*ddxn + vt*ddxt, partial vertical.
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

/* ============================ kernels =============================== */
template <int V, int BX, int BY>
__global__ void gpu_unblocked(
    double *__restrict__ out, const double *__restrict__ vn,
    const double *__restrict__ vt, const double *__restrict__ ddxn,
    const double *__restrict__ ddxt, int N_e, int nlev, int jk0, int jk1) {
  int je = blockIdx.x*BX + threadIdx.x;
  int jk = blockIdx.y*BY + threadIdx.y;
  if (je >= N_e || jk < jk0 || jk >= jk1) return;
  int c = IC<V>(je, jk, N_e, nlev);
  out[c] = vn[c]*ddxn[c] + vt[c]*ddxt[c];
}

template <int B, int BX, int BY>
__global__ void gpu_blocked(
    double *__restrict__ out, const double *__restrict__ vn,
    const double *__restrict__ vt, const double *__restrict__ ddxn,
    const double *__restrict__ ddxt, int N_e, int nlev, int jk0, int jk1) {
  int je = blockIdx.x*BX + threadIdx.x;
  int jk = blockIdx.y*BY + threadIdx.y;
  if (je >= N_e || jk < jk0 || jk >= jk1) return;
  int c = IC_blocked(je, jk, B, nlev);
  out[c] = vn[c]*ddxn[c] + vt[c]*ddxt[c];
}

template <int TX, int TY, int BX, int BY>
__global__ void gpu_tiled(
    double *__restrict__ out, const double *__restrict__ vn,
    const double *__restrict__ vt, const double *__restrict__ ddxn,
    const double *__restrict__ ddxt, int N_e, int nlev, int jk0, int jk1) {
  int je = blockIdx.x*BX + threadIdx.x;
  int jk = blockIdx.y*BY + threadIdx.y;
  if (je >= N_e || jk < jk0 || jk >= jk1) return;
  int c = IC_tiled(je, jk, TX, TY, N_e, nlev);
  out[c] = vn[c]*ddxn[c] + vt[c]*ddxt[c];
}

/* ============================ cpu reference ========================= */
template <int V>
static void cpu_ref(double *out, const double *vn, const double *vt,
                    const double *ddxn, const double *ddxt,
                    int N_e, int nlev, int jk0, int jk1) {
  for (int jk = jk0; jk < jk1; jk++)
    for (int je = 0; je < N_e; je++) {
      int c = IC<V>(je,jk,N_e,nlev);
      out[c] = vn[c]*ddxn[c] + vt[c]*ddxt[c];
    }
}

/* ============================ config tables ========================= */
struct GCfg { int BX, BY; const char *tag; };
static const GCfg GCFG[] = {
  { 32,  4, "bx32_by04"},
  { 32,  8, "bx32_by08"},
  { 64,  4, "bx64_by04"},
  { 16, 16, "bx16_by16"},
  {128,  1, "bx128_by01"}
};
static constexpr int N_GCFG = sizeof(GCFG)/sizeof(GCFG[0]);

/* ============================ launchers ============================= */
static void launch_unblocked(int V, int bxi, double *d_out, const double *d_vn,
                             const double *d_vt, const double *d_ddxn, const double *d_ddxt,
                             int N_e, int nlev, int jk0, int jk1) {
  int BX = GCFG[bxi].BX, BY = GCFG[bxi].BY;
  dim3 T(BX, BY, 1);
  dim3 G((N_e + BX - 1)/BX, (nlev + BY - 1)/BY, 1);
  switch (V*100 + bxi) {
#define CASE_V(V_)                                                                       \
    case V_*100+0: gpu_unblocked<V_, 32, 4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break; \
    case V_*100+1: gpu_unblocked<V_, 32, 8><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break; \
    case V_*100+2: gpu_unblocked<V_, 64, 4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break; \
    case V_*100+3: gpu_unblocked<V_, 16,16><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break; \
    case V_*100+4: gpu_unblocked<V_,128, 1><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    CASE_V(1) CASE_V(2) CASE_V(3) CASE_V(4)
#undef CASE_V
  }
}
static void launch_blocked(int bi, int bxi, double *d_out, const double *d_vn,
                           const double *d_vt, const double *d_ddxn, const double *d_ddxt,
                           int N_e, int nlev, int jk0, int jk1) {
  int BX = GCFG[bxi].BX, BY = GCFG[bxi].BY;
  dim3 T(BX, BY, 1);
  dim3 G((N_e + BX - 1)/BX, (nlev + BY - 1)/BY, 1);
  switch (bi*100 + bxi) {
#define CASE_B(B_)                                                                     \
    case B_##_0: gpu_blocked<B_, 32, 4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;    \
    case B_##_1: gpu_blocked<B_, 32, 8><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;    \
    case B_##_2: gpu_blocked<B_, 64, 4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;    \
    case B_##_3: gpu_blocked<B_, 16,16><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;    \
    case B_##_4: gpu_blocked<B_,128, 1><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    /* fall-through manual below; avoid token-paste of int literals */
#undef CASE_B
    case 0*100+0: gpu_blocked<8, 32,4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 0*100+1: gpu_blocked<8, 32,8><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 0*100+2: gpu_blocked<8, 64,4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 0*100+3: gpu_blocked<8, 16,16><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 0*100+4: gpu_blocked<8,128,1><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 1*100+0: gpu_blocked<16,32,4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 1*100+1: gpu_blocked<16,32,8><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 1*100+2: gpu_blocked<16,64,4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 1*100+3: gpu_blocked<16,16,16><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 1*100+4: gpu_blocked<16,128,1><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 2*100+0: gpu_blocked<32,32,4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 2*100+1: gpu_blocked<32,32,8><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 2*100+2: gpu_blocked<32,64,4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 2*100+3: gpu_blocked<32,16,16><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 2*100+4: gpu_blocked<32,128,1><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 3*100+0: gpu_blocked<64,32,4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 3*100+1: gpu_blocked<64,32,8><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 3*100+2: gpu_blocked<64,64,4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 3*100+3: gpu_blocked<64,16,16><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 3*100+4: gpu_blocked<64,128,1><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 4*100+0: gpu_blocked<128,32,4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 4*100+1: gpu_blocked<128,32,8><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 4*100+2: gpu_blocked<128,64,4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 4*100+3: gpu_blocked<128,16,16><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 4*100+4: gpu_blocked<128,128,1><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
  }
}

template <int TX, int TY>
static void launch_tiled_inner(int bxi, dim3 G, dim3 T, double *d_out,
                               const double *d_vn, const double *d_vt,
                               const double *d_ddxn, const double *d_ddxt,
                               int N_e, int nlev, int jk0, int jk1) {
  switch (bxi) {
    case 0: gpu_tiled<TX,TY,32,4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 1: gpu_tiled<TX,TY,32,8><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 2: gpu_tiled<TX,TY,64,4><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 3: gpu_tiled<TX,TY,16,16><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
    case 4: gpu_tiled<TX,TY,128,1><<<G,T>>>(d_out,d_vn,d_vt,d_ddxn,d_ddxt,N_e,nlev,jk0,jk1); break;
  }
}
static void launch_tiled(int txi, int tyi, int bxi, double *d_out,
                         const double *d_vn, const double *d_vt,
                         const double *d_ddxn, const double *d_ddxt,
                         int N_e, int nlev, int jk0, int jk1) {
  int BX = GCFG[bxi].BX, BY = GCFG[bxi].BY;
  dim3 T(BX, BY, 1);
  dim3 G((N_e + BX - 1)/BX, (nlev + BY - 1)/BY, 1);
#define T4(TX_, TY_) launch_tiled_inner<TX_, TY_>(bxi, G, T, d_out, d_vn, d_vt, d_ddxn, d_ddxt, N_e, nlev, jk0, jk1)
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

/* ============================ runner ============================== */
static void run_configs_unblocked(
    FILE *fcsv, int V, BenchData &bd, int nlev, int nlev_end,
    const char *dist, double *d_out, const double *d_vn, const double *d_vt,
    const double *d_ddxn, const double *d_ddxt,
    cudaEvent_t ev0, cudaEvent_t ev1, std::vector<double> &h_ref_rm,
    std::vector<double> &h_gpu_out) {
  int jk0 = nflatlev_for(nlev), jk1 = nlev_end;
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    /* warmup */
    for (int r = 0; r < GPU_WARMUP; r++) {
      launch_unblocked(V, bxi, d_out, d_vn, d_vt, d_ddxn, d_ddxt, bd.N_e, bd.nlev, jk0, jk1);
    }
    CUDA_CHECK(cudaDeviceSynchronize());
    /* timed */
    for (int r = 0; r < GPU_NRUNS; r++) {
      CUDA_CHECK(cudaEventRecord(ev0));
      launch_unblocked(V, bxi, d_out, d_vn, d_vt, d_ddxn, d_ddxt, bd.N_e, bd.nlev, jk0, jk1);
      CUDA_CHECK(cudaEventRecord(ev1));
      CUDA_CHECK(cudaEventSynchronize(ev1));
      float ms; CUDA_CHECK(cudaEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,%d,%d,%d,%s,V%d_%s,run%d,%d,%.6f\n",
              V, bd.nlev, bd.N_e, dist, V, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
  (void)h_ref_rm; (void)h_gpu_out;
}
static void run_configs_blocked(
    FILE *fcsv, int bi, BenchData &bd, int nlev, int nlev_end,
    const char *dist, double *d_out, const double *d_vn, const double *d_vt,
    const double *d_ddxn, const double *d_ddxt,
    cudaEvent_t ev0, cudaEvent_t ev1) {
  int B = BLOCK_SIZES[bi];
  int jk0 = nflatlev_for(nlev), jk1 = nlev_end;
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++)
      launch_blocked(bi, bxi, d_out, d_vn, d_vt, d_ddxn, d_ddxt, bd.N_e, bd.nlev, jk0, jk1);
    CUDA_CHECK(cudaDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      CUDA_CHECK(cudaEventRecord(ev0));
      launch_blocked(bi, bxi, d_out, d_vn, d_vt, d_ddxn, d_ddxt, bd.N_e, bd.nlev, jk0, jk1);
      CUDA_CHECK(cudaEventRecord(ev1));
      CUDA_CHECK(cudaEventSynchronize(ev1));
      float ms; CUDA_CHECK(cudaEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,0,%d,%d,%s,B%d_%s,run%d,%d,%.6f\n",
              bd.nlev, bd.N_e, dist, B, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_tiled(
    FILE *fcsv, int txi, int tyi, BenchData &bd, int nlev, int nlev_end,
    const char *dist, double *d_out, const double *d_vn, const double *d_vt,
    const double *d_ddxn, const double *d_ddxt,
    cudaEvent_t ev0, cudaEvent_t ev1) {
  int TX = TILE_X_VALUES[txi], TY = TILE_Y_VALUES[tyi + 1];
  int jk0 = nflatlev_for(nlev), jk1 = nlev_end;
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    for (int r = 0; r < GPU_WARMUP; r++)
      launch_tiled(txi, tyi, bxi, d_out, d_vn, d_vt, d_ddxn, d_ddxt, bd.N_e, bd.nlev, jk0, jk1);
    CUDA_CHECK(cudaDeviceSynchronize());
    for (int r = 0; r < GPU_NRUNS; r++) {
      CUDA_CHECK(cudaEventRecord(ev0));
      launch_tiled(txi, tyi, bxi, d_out, d_vn, d_vt, d_ddxn, d_ddxt, bd.N_e, bd.nlev, jk0, jk1);
      CUDA_CHECK(cudaEventRecord(ev1));
      CUDA_CHECK(cudaEventSynchronize(ev1));
      float ms; CUDA_CHECK(cudaEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,0,%d,%d,%s,t%02dx%02d_%s,run%d,%d,%.6f\n",
              bd.nlev, bd.N_e, dist, TX, TY, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}

/* ============================ main ================================ */
int main(int argc, char *argv[]) {
  const char *csv = (argc >= 2) ? argv[1] : "z_w_concorr_me_gpu.csv";
  FILE *fcsv = fopen(csv, "w"); if (!fcsv) { perror("fopen"); return 1; }
  fprintf(fcsv, "backend,V,nlev,N_e,cell_dist,config_label,run_label,run_id,time_ms\n");

  int step = (argc >= 3) ? atoi(argv[2]) : 9;
  std::string gp = icon_global_path(step), pp = icon_patch_path(step);
  int icon_nproma = icon_read_nproma(gp.c_str());
  IconEdgeData ied;
  bool have_exact = (icon_nproma > 0) && icon_load_patch(pp.c_str(), icon_nproma, ied);
  int N_e = have_exact ? ied.n_edges : 122880;

  double *d_vn,*d_vt,*d_xn,*d_xt,*d_out;
  size_t sz = (size_t)N_e * NLEVS[0];
  CUDA_CHECK(cudaMalloc(&d_vn, sz*8)); CUDA_CHECK(cudaMalloc(&d_vt, sz*8));
  CUDA_CHECK(cudaMalloc(&d_xn, sz*8)); CUDA_CHECK(cudaMalloc(&d_xt, sz*8));
  CUDA_CHECK(cudaMalloc(&d_out, sz*8));
  cudaEvent_t ev0, ev1; cudaEventCreate(&ev0); cudaEventCreate(&ev1);

  const char *dists[3] = {"uniform", "normal_var1", "exact"};
  int ndists = have_exact ? 3 : 2;

  for (int ni = 0; ni < N_NLEVS; ni++) {
    int nlev = NLEVS[ni], nlev_end = nlev;

    /* unblocked V1-V4 */
    for (int di = 0; di < ndists; di++) {
      for (int V = 1; V <= 4; V++) {
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant(V);
        CUDA_CHECK(cudaMemcpy(d_vn, bd.h_vn, sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_vt, bd.h_vt, sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_xn, bd.h_ddxn, sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_xt, bd.h_ddxt, sz*8, cudaMemcpyHostToDevice));
        std::vector<double> ref, got;
        run_configs_unblocked(fcsv, V, bd, nlev, nlev_end, dists[di],
                              d_out, d_vn, d_vt, d_xn, d_xt, ev0, ev1, ref, got);
        fflush(fcsv); bd.free_all();
      }
    }

    /* blocked */
    for (int di = 0; di < ndists; di++) {
      for (int bi = 0; bi < N_BLOCK_SIZES; bi++) {
        int B = BLOCK_SIZES[bi];
        if (N_e % B != 0) { printf("SKIP GPU B=%d\n", B); continue; }
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant_blocked(B);
        CUDA_CHECK(cudaMemcpy(d_vn, bd.h_vn, sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_vt, bd.h_vt, sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_xn, bd.h_ddxn, sz*8, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_xt, bd.h_ddxt, sz*8, cudaMemcpyHostToDevice));
        run_configs_blocked(fcsv, bi, bd, nlev, nlev_end, dists[di],
                            d_out, d_vn, d_vt, d_xn, d_xt, ev0, ev1);
        fflush(fcsv); bd.free_all();
      }
    }

    /* tiled */
    for (int di = 0; di < ndists; di++) {
      for (int txi = 0; txi < N_TILE_X; txi++) {
        int TX = TILE_X_VALUES[txi];
        if (N_e % TX != 0) continue;
        for (int tyi = 0; tyi < 4; tyi++) {
          int TY = TILE_Y_VALUES[tyi + 1];
          if (nlev % TY != 0) continue;
          BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
          bd.set_variant_tiled(TX, TY);
          CUDA_CHECK(cudaMemcpy(d_vn, bd.h_vn, sz*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_vt, bd.h_vt, sz*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_xn, bd.h_ddxn, sz*8, cudaMemcpyHostToDevice));
          CUDA_CHECK(cudaMemcpy(d_xt, bd.h_ddxt, sz*8, cudaMemcpyHostToDevice));
          run_configs_tiled(fcsv, txi, tyi, bd, nlev, nlev_end, dists[di],
                            d_out, d_vn, d_vt, d_xn, d_xt, ev0, ev1);
          fflush(fcsv); bd.free_all();
        }
      }
    }
  }

  cudaFree(d_vn); cudaFree(d_vt); cudaFree(d_xn); cudaFree(d_xt); cudaFree(d_out);
  cudaEventDestroy(ev0); cudaEventDestroy(ev1);
  if (have_exact) ied.free_all();
  fclose(fcsv);
  return 0;
}
