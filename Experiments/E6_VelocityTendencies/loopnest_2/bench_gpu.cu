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

#include "../../common/gpu_compat.cuh"
#include "bench_common.h"
#include "../loopnest_1/icon_data_loader.h"
#include "../../common/jacobi_flush_gpu.cuh"

/* GPU_CHECK, WARMUP, NRUNS are provided by the shared headers above. */

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
static void cpu_ref_V(int V, double *out, const double *vn, const double *vt,
                      const double *ddxn, const double *ddxt,
                      int N_e, int nlev, int jk0, int jk1) {
  switch (V) {
    case 1: cpu_ref<1>(out, vn, vt, ddxn, ddxt, N_e, nlev, jk0, jk1); break;
    case 2: cpu_ref<2>(out, vn, vt, ddxn, ddxt, N_e, nlev, jk0, jk1); break;
    case 3: cpu_ref<3>(out, vn, vt, ddxn, ddxt, N_e, nlev, jk0, jk1); break;
    case 4: cpu_ref<4>(out, vn, vt, ddxn, ddxt, N_e, nlev, jk0, jk1); break;
  }
}
static void cpu_ref_blocked(int B, double *out, const double *vn, const double *vt,
                            const double *ddxn, const double *ddxt,
                            int N_e, int nlev, int jk0, int jk1) {
  for (int jk = jk0; jk < jk1; jk++)
    for (int je = 0; je < N_e; je++) {
      int c = IC_blocked(je, jk, B, nlev);
      out[c] = vn[c]*ddxn[c] + vt[c]*ddxt[c];
    }
}
static void cpu_ref_tiled(int TX, int TY,
                          double *out, const double *vn, const double *vt,
                          const double *ddxn, const double *ddxt,
                          int N_e, int nlev, int jk0, int jk1) {
  for (int jk = jk0; jk < jk1; jk++)
    for (int je = 0; je < N_e; je++) {
      int c = IC_tiled(je, jk, TX, TY, N_e, nlev);
      out[c] = vn[c]*ddxn[c] + vt[c]*ddxt[c];
    }
}

/* Element-wise comparison with mixed absolute/relative tolerance.      */
/* Returns true iff every element satisfies |got-ref| <= atol + rtol*|ref|. */
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
  /* 3 block sizes x 5 (BX,BY) configs = 15 hand-written cases below.   */
  switch (bi*100 + bxi) {
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
    case 0*4+2: T4(8,32);  break;
    case 1*4+0: T4(16,8);  break;  case 1*4+1: T4(16,16); break;
    case 1*4+2: T4(16,32); break;
    case 2*4+0: T4(32,8);  break;  case 2*4+1: T4(32,16); break;
    case 2*4+2: T4(32,32); break;
  }
#undef T4
}

/* ============================ runner ============================== */
/* Each runner:                                                         */
/*   1. precomputes the CPU reference into h_ref (full sz);             */
/*   2. for each (bxi) config:                                          */
/*        - skips if the (grid, block) exceeds device launch limits;    */
/*        - clears d_out and runs WARMUP reps;                          */
/*        - downloads d_out and compares to h_ref; skips timed loop on  */
/*          FAIL so only verified configurations produce CSV rows;      */
/*        - then runs the NRUNS timed reps with a canonical flush       */
/*          between each.                                                */
static void run_configs_unblocked(
    FILE *fcsv, int V, BenchData &bd, int nlev, int nlev_end,
    const char *dist, double *d_out, const double *d_vn, const double *d_vt,
    const double *d_ddxn, const double *d_ddxt,
    gpuEvent_t ev0, gpuEvent_t ev1,
    double *h_ref, double *h_gpu_out) {
  int jk0 = nflatlev_for(nlev), jk1 = nlev_end;
  size_t sz = (size_t)bd.N_e * bd.nlev;
  std::memset(h_ref, 0, sz * sizeof(double));
  cpu_ref_V(V, h_ref, bd.h_vn, bd.h_vt, bd.h_ddxn, bd.h_ddxt,
            bd.N_e, bd.nlev, jk0, jk1);
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    int BX = GCFG[bxi].BX, BY = GCFG[bxi].BY;
    dim3 T(BX, BY, 1);
    dim3 G((bd.N_e + BX - 1)/BX, (bd.nlev + BY - 1)/BY, 1);
    if (!gpu_launch_valid(G, T)) {
      printf("SKIP V=%d cfg=%s: invalid launch grid=%ux%u block=%ux%u\n",
             V, GCFG[bxi].tag, G.x, G.y, T.x, T.y);
      continue;
    }
    GPU_CHECK(gpuMemset(d_out, 0, sz * 8));
    for (int r = 0; r < WARMUP; r++) {
      launch_unblocked(V, bxi, d_out, d_vn, d_vt, d_ddxn, d_ddxt, bd.N_e, bd.nlev, jk0, jk1);
    }
    GPU_CHECK(gpuDeviceSynchronize());
    GPU_CHECK(gpuMemcpy(h_gpu_out, d_out, sz * 8, gpuMemcpyDeviceToHost));
    double mr = 0.0;
    if (!verify_close(h_gpu_out, h_ref, sz, 1e-8, 1e-12, &mr)) {
      printf("FAIL V=%d cfg=%s max_rel=%.3e\n", V, GCFG[bxi].tag, mr);
      continue;
    }
    if (bxi == 0) printf("OK   V=%d dist=%s max_rel=%.3e\n", V, dist, mr);
    for (int r = 0; r < NRUNS; r++) {
      flush_jacobi_gpu(); GPU_CHECK(gpuEventRecord(ev0));
      launch_unblocked(V, bxi, d_out, d_vn, d_vt, d_ddxn, d_ddxt, bd.N_e, bd.nlev, jk0, jk1);
      GPU_CHECK(gpuEventRecord(ev1));
      GPU_CHECK(gpuEventSynchronize(ev1));
      float ms; GPU_CHECK(gpuEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,%d,%d,%d,%s,V%d_%s,run%d,%d,%.6f\n",
              V, bd.nlev, bd.N_e, dist, V, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_blocked(
    FILE *fcsv, int bi, BenchData &bd, int nlev, int nlev_end,
    const char *dist, double *d_out, const double *d_vn, const double *d_vt,
    const double *d_ddxn, const double *d_ddxt,
    gpuEvent_t ev0, gpuEvent_t ev1,
    double *h_ref, double *h_gpu_out) {
  int B = BLOCK_SIZES[bi];
  int jk0 = nflatlev_for(nlev), jk1 = nlev_end;
  size_t sz = (size_t)bd.N_e * bd.nlev;
  std::memset(h_ref, 0, sz * sizeof(double));
  cpu_ref_blocked(B, h_ref, bd.h_vn, bd.h_vt, bd.h_ddxn, bd.h_ddxt,
                  bd.N_e, bd.nlev, jk0, jk1);
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    int BX = GCFG[bxi].BX, BY = GCFG[bxi].BY;
    dim3 T(BX, BY, 1);
    dim3 G((bd.N_e + BX - 1)/BX, (bd.nlev + BY - 1)/BY, 1);
    if (!gpu_launch_valid(G, T)) {
      printf("SKIP B=%d cfg=%s: invalid launch grid=%ux%u block=%ux%u\n",
             B, GCFG[bxi].tag, G.x, G.y, T.x, T.y);
      continue;
    }
    GPU_CHECK(gpuMemset(d_out, 0, sz * 8));
    for (int r = 0; r < WARMUP; r++)
      launch_blocked(bi, bxi, d_out, d_vn, d_vt, d_ddxn, d_ddxt, bd.N_e, bd.nlev, jk0, jk1);
    GPU_CHECK(gpuDeviceSynchronize());
    GPU_CHECK(gpuMemcpy(h_gpu_out, d_out, sz * 8, gpuMemcpyDeviceToHost));
    double mr = 0.0;
    if (!verify_close(h_gpu_out, h_ref, sz, 1e-8, 1e-12, &mr)) {
      printf("FAIL B=%d cfg=%s max_rel=%.3e\n", B, GCFG[bxi].tag, mr);
      continue;
    }
    if (bxi == 0) printf("OK   B=%d dist=%s max_rel=%.3e\n", B, dist, mr);
    for (int r = 0; r < NRUNS; r++) {
      flush_jacobi_gpu(); GPU_CHECK(gpuEventRecord(ev0));
      launch_blocked(bi, bxi, d_out, d_vn, d_vt, d_ddxn, d_ddxt, bd.N_e, bd.nlev, jk0, jk1);
      GPU_CHECK(gpuEventRecord(ev1));
      GPU_CHECK(gpuEventSynchronize(ev1));
      float ms; GPU_CHECK(gpuEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,0,%d,%d,%s,B%d_%s,run%d,%d,%.6f\n",
              bd.nlev, bd.N_e, dist, B, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_tiled(
    FILE *fcsv, int txi, int tyi, BenchData &bd, int nlev, int nlev_end,
    const char *dist, double *d_out, const double *d_vn, const double *d_vt,
    const double *d_ddxn, const double *d_ddxt,
    gpuEvent_t ev0, gpuEvent_t ev1,
    double *h_ref, double *h_gpu_out) {
  int TX = TILE_X_VALUES[txi], TY = TILE_Y_VALUES[tyi + 1];
  int jk0 = nflatlev_for(nlev), jk1 = nlev_end;
  size_t sz = (size_t)bd.N_e * bd.nlev;
  std::memset(h_ref, 0, sz * sizeof(double));
  cpu_ref_tiled(TX, TY, h_ref, bd.h_vn, bd.h_vt, bd.h_ddxn, bd.h_ddxt,
                bd.N_e, bd.nlev, jk0, jk1);
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    int BX = GCFG[bxi].BX, BY = GCFG[bxi].BY;
    dim3 T(BX, BY, 1);
    dim3 G((bd.N_e + BX - 1)/BX, (bd.nlev + BY - 1)/BY, 1);
    if (!gpu_launch_valid(G, T)) {
      printf("SKIP TX=%d TY=%d cfg=%s: invalid launch grid=%ux%u block=%ux%u\n",
             TX, TY, GCFG[bxi].tag, G.x, G.y, T.x, T.y);
      continue;
    }
    GPU_CHECK(gpuMemset(d_out, 0, sz * 8));
    for (int r = 0; r < WARMUP; r++)
      launch_tiled(txi, tyi, bxi, d_out, d_vn, d_vt, d_ddxn, d_ddxt, bd.N_e, bd.nlev, jk0, jk1);
    GPU_CHECK(gpuDeviceSynchronize());
    GPU_CHECK(gpuMemcpy(h_gpu_out, d_out, sz * 8, gpuMemcpyDeviceToHost));
    double mr = 0.0;
    if (!verify_close(h_gpu_out, h_ref, sz, 1e-8, 1e-12, &mr)) {
      printf("FAIL TX=%d TY=%d cfg=%s max_rel=%.3e\n", TX, TY, GCFG[bxi].tag, mr);
      continue;
    }
    if (bxi == 0) printf("OK   TX=%d TY=%d dist=%s max_rel=%.3e\n", TX, TY, dist, mr);
    for (int r = 0; r < NRUNS; r++) {
      flush_jacobi_gpu(); GPU_CHECK(gpuEventRecord(ev0));
      launch_tiled(txi, tyi, bxi, d_out, d_vn, d_vt, d_ddxn, d_ddxt, bd.N_e, bd.nlev, jk0, jk1);
      GPU_CHECK(gpuEventRecord(ev1));
      GPU_CHECK(gpuEventSynchronize(ev1));
      float ms; GPU_CHECK(gpuEventElapsedTime(&ms, ev0, ev1));
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
  int max_nlev = 0;
  for (int ni = 0; ni < N_NLEVS; ni++) if (NLEVS[ni] > max_nlev) max_nlev = NLEVS[ni];
  size_t sz_max = (size_t)N_e * max_nlev;
  GPU_CHECK(gpuMalloc(&d_vn, sz_max*8)); GPU_CHECK(gpuMalloc(&d_vt, sz_max*8));
  GPU_CHECK(gpuMalloc(&d_xn, sz_max*8)); GPU_CHECK(gpuMalloc(&d_xt, sz_max*8));
  GPU_CHECK(gpuMalloc(&d_out, sz_max*8));
  std::vector<double> h_ref(sz_max), h_gpu_out(sz_max);
  gpuEvent_t ev0, ev1; GPU_CHECK(gpuEventCreate(&ev0)); GPU_CHECK(gpuEventCreate(&ev1));

  const char *dists[3] = {"uniform", "normal_var1", "exact"};
  int ndists = have_exact ? 3 : 2;

  for (int ni = 0; ni < N_NLEVS; ni++) {
    int nlev = NLEVS[ni], nlev_end = nlev;
    size_t sz = (size_t)N_e * nlev;

    /* unblocked V1-V4 */
    for (int di = 0; di < ndists; di++) {
      for (int V = 1; V <= 4; V++) {
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant(V);
        GPU_CHECK(gpuMemcpy(d_vn, bd.h_vn, sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_vt, bd.h_vt, sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_xn, bd.h_ddxn, sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_xt, bd.h_ddxt, sz*8, gpuMemcpyHostToDevice));
        run_configs_unblocked(fcsv, V, bd, nlev, nlev_end, dists[di],
                              d_out, d_vn, d_vt, d_xn, d_xt, ev0, ev1,
                              h_ref.data(), h_gpu_out.data());
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
        GPU_CHECK(gpuMemcpy(d_vn, bd.h_vn, sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_vt, bd.h_vt, sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_xn, bd.h_ddxn, sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_xt, bd.h_ddxt, sz*8, gpuMemcpyHostToDevice));
        run_configs_blocked(fcsv, bi, bd, nlev, nlev_end, dists[di],
                            d_out, d_vn, d_vt, d_xn, d_xt, ev0, ev1,
                            h_ref.data(), h_gpu_out.data());
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
          GPU_CHECK(gpuMemcpy(d_vn, bd.h_vn, sz*8, gpuMemcpyHostToDevice));
          GPU_CHECK(gpuMemcpy(d_vt, bd.h_vt, sz*8, gpuMemcpyHostToDevice));
          GPU_CHECK(gpuMemcpy(d_xn, bd.h_ddxn, sz*8, gpuMemcpyHostToDevice));
          GPU_CHECK(gpuMemcpy(d_xt, bd.h_ddxt, sz*8, gpuMemcpyHostToDevice));
          run_configs_tiled(fcsv, txi, tyi, bd, nlev, nlev_end, dists[di],
                            d_out, d_vn, d_vt, d_xn, d_xt, ev0, ev1,
                            h_ref.data(), h_gpu_out.data());
          fflush(fcsv); bd.free_all();
        }
      }
    }
  }

  GPU_CHECK(gpuFree(d_vn)); GPU_CHECK(gpuFree(d_vt)); GPU_CHECK(gpuFree(d_xn)); GPU_CHECK(gpuFree(d_xt)); GPU_CHECK(gpuFree(d_out));
  GPU_CHECK(gpuEventDestroy(ev0)); GPU_CHECK(gpuEventDestroy(ev1));
  if (have_exact) ied.free_all();
  fclose(fcsv);
  return 0;
}
