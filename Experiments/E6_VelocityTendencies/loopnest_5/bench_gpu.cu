/*
 * bench_gpu.cu -- loopnest_5 (Nest 2) GPU (CUDA) benchmark.
 * Horizontal-only boundary kernel.
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

/* ============================ cpu reference ========================= */
/* Three outputs: vnie (ct + cb), zvt (ct), zk (ct). Mirror of the GPU. */
template <int V>
static void cpu_ref_unblocked(double *vnie, double *zvt, double *zk,
                              const double *vn, const double *vt,
                              const double *ubc, const double *wgt,
                              int N_e, int nlev, int nlevp1) {
  (void)nlev;
  for (int je = 0; je < N_e; je++) {
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
}
static void cpu_ref_V(int V, double *vnie, double *zvt, double *zk,
                      const double *vn, const double *vt,
                      const double *ubc, const double *wgt,
                      int N_e, int nlev, int nlevp1) {
  switch (V) {
    case 1: cpu_ref_unblocked<1>(vnie, zvt, zk, vn, vt, ubc, wgt, N_e, nlev, nlevp1); break;
    case 2: cpu_ref_unblocked<2>(vnie, zvt, zk, vn, vt, ubc, wgt, N_e, nlev, nlevp1); break;
    case 3: cpu_ref_unblocked<3>(vnie, zvt, zk, vn, vt, ubc, wgt, N_e, nlev, nlevp1); break;
    case 4: cpu_ref_unblocked<4>(vnie, zvt, zk, vn, vt, ubc, wgt, N_e, nlev, nlevp1); break;
  }
}
static void cpu_ref_blocked(int B, double *vnie, double *zvt, double *zk,
                            const double *vn, const double *vt,
                            const double *ubc, const double *wgt,
                            int N_e, int nlev, int nlevp1) {
  (void)nlev;
  for (int je = 0; je < N_e; je++) {
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

/* Verifies 3 outputs (vnie, zvt, zk); kernel is idempotent so no reset    */
/* is required between verify launch and subsequent warmup/timed launches. */
static void run_configs_unblocked(FILE *fcsv, int V, BenchData &bd, const char *dist,
    double *d_vnie, double *d_zvt, double *d_zk, const double *d_vn, const double *d_vt,
    const double *d_ubc, const double *d_wgt, gpuEvent_t ev0, gpuEvent_t ev1,
    double *h_ref_a, double *h_ref_b, double *h_ref_c, double *h_gpu_out) {
  int N_e = bd.N_e, nlev = bd.nlev, nlevp1 = bd.nlevp1;
  size_t sz = (size_t)N_e * nlevp1;
  /* Initialize the reference copies from the BenchData inputs; the kernel */
  /* only overwrites ct and cb indices, so the rest must match the initial */
  /* state that was gpuMemcpy'd to d_vnie / d_zvt / d_zk.                 */
  std::memcpy(h_ref_a, bd.h_vnie, sz * sizeof(double));
  std::memcpy(h_ref_b, bd.h_zvt,  sz * sizeof(double));
  std::memcpy(h_ref_c, bd.h_zk,   sz * sizeof(double));
  cpu_ref_V(V, h_ref_a, h_ref_b, h_ref_c, bd.h_vn, bd.h_vt, bd.h_ubc, bd.h_wgt,
            N_e, nlev, nlevp1);
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    int BX = GCFG[bxi].BX;
    dim3 T(BX, 1, 1);
    dim3 G((N_e + BX - 1)/BX, 1, 1);
    if (!gpu_launch_valid(G, T)) {
      printf("SKIP V=%d cfg=%s: invalid launch grid=%u block=%u\n",
             V, GCFG[bxi].tag, G.x, T.x);
      continue;
    }
    launch_unblocked(V, bxi, ARGS, N_e, nlev, nlevp1);
    GPU_CHECK(gpuDeviceSynchronize());
    double mr = 0.0; bool ok = true;
    GPU_CHECK(gpuMemcpy(h_gpu_out, d_vnie, sz*8, gpuMemcpyDeviceToHost));
    ok = ok && verify_close(h_gpu_out, h_ref_a, sz, 1e-8, 1e-12, &mr);
    if (ok) { GPU_CHECK(gpuMemcpy(h_gpu_out, d_zvt, sz*8, gpuMemcpyDeviceToHost));
              ok = verify_close(h_gpu_out, h_ref_b, sz, 1e-8, 1e-12, &mr); }
    if (ok) { GPU_CHECK(gpuMemcpy(h_gpu_out, d_zk,  sz*8, gpuMemcpyDeviceToHost));
              ok = verify_close(h_gpu_out, h_ref_c, sz, 1e-8, 1e-12, &mr); }
    if (!ok) {
      printf("FAIL V=%d cfg=%s max_rel=%.3e\n", V, GCFG[bxi].tag, mr);
      continue;
    }
    if (bxi == 0) printf("OK   V=%d dist=%s max_rel=%.3e\n", V, dist, mr);
    for (int r = 0; r < WARMUP; r++)
      launch_unblocked(V, bxi, ARGS, N_e, nlev, nlevp1);
    GPU_CHECK(gpuDeviceSynchronize());
    for (int r = 0; r < NRUNS; r++) {
      flush_jacobi_gpu(); GPU_CHECK(gpuEventRecord(ev0));
      launch_unblocked(V, bxi, ARGS, N_e, nlev, nlevp1);
      GPU_CHECK(gpuEventRecord(ev1));
      GPU_CHECK(gpuEventSynchronize(ev1));
      float ms; GPU_CHECK(gpuEventElapsedTime(&ms, ev0, ev1));
      fprintf(fcsv, "gpu,%d,%d,%d,%s,V%d_%s,run%d,%d,%.6f\n",
              V, bd.nlev, bd.N_e, dist, V, GCFG[bxi].tag, r, r, (double)ms);
    }
  }
}
static void run_configs_blocked(FILE *fcsv, int bi, BenchData &bd, const char *dist,
    double *d_vnie, double *d_zvt, double *d_zk, const double *d_vn, const double *d_vt,
    const double *d_ubc, const double *d_wgt, gpuEvent_t ev0, gpuEvent_t ev1,
    double *h_ref_a, double *h_ref_b, double *h_ref_c, double *h_gpu_out) {
  int B = BLOCK_SIZES[bi];
  int N_e = bd.N_e, nlev = bd.nlev, nlevp1 = bd.nlevp1;
  size_t sz = (size_t)N_e * nlevp1;
  std::memcpy(h_ref_a, bd.h_vnie, sz * sizeof(double));
  std::memcpy(h_ref_b, bd.h_zvt,  sz * sizeof(double));
  std::memcpy(h_ref_c, bd.h_zk,   sz * sizeof(double));
  cpu_ref_blocked(B, h_ref_a, h_ref_b, h_ref_c, bd.h_vn, bd.h_vt, bd.h_ubc, bd.h_wgt,
                  N_e, nlev, nlevp1);
  for (int bxi = 0; bxi < N_GCFG; bxi++) {
    int BX = GCFG[bxi].BX;
    dim3 T(BX, 1, 1);
    dim3 G((N_e + BX - 1)/BX, 1, 1);
    if (!gpu_launch_valid(G, T)) {
      printf("SKIP B=%d cfg=%s: invalid launch grid=%u block=%u\n",
             B, GCFG[bxi].tag, G.x, T.x);
      continue;
    }
    launch_blocked(bi, bxi, ARGS, N_e, nlev, nlevp1);
    GPU_CHECK(gpuDeviceSynchronize());
    double mr = 0.0; bool ok = true;
    GPU_CHECK(gpuMemcpy(h_gpu_out, d_vnie, sz*8, gpuMemcpyDeviceToHost));
    ok = ok && verify_close(h_gpu_out, h_ref_a, sz, 1e-8, 1e-12, &mr);
    if (ok) { GPU_CHECK(gpuMemcpy(h_gpu_out, d_zvt, sz*8, gpuMemcpyDeviceToHost));
              ok = verify_close(h_gpu_out, h_ref_b, sz, 1e-8, 1e-12, &mr); }
    if (ok) { GPU_CHECK(gpuMemcpy(h_gpu_out, d_zk,  sz*8, gpuMemcpyDeviceToHost));
              ok = verify_close(h_gpu_out, h_ref_c, sz, 1e-8, 1e-12, &mr); }
    if (!ok) {
      printf("FAIL B=%d cfg=%s max_rel=%.3e\n", B, GCFG[bxi].tag, mr);
      continue;
    }
    if (bxi == 0) printf("OK   B=%d dist=%s max_rel=%.3e\n", B, dist, mr);
    for (int r = 0; r < WARMUP; r++)
      launch_blocked(bi, bxi, ARGS, N_e, nlev, nlevp1);
    GPU_CHECK(gpuDeviceSynchronize());
    for (int r = 0; r < NRUNS; r++) {
      flush_jacobi_gpu(); GPU_CHECK(gpuEventRecord(ev0));
      launch_blocked(bi, bxi, ARGS, N_e, nlev, nlevp1);
      GPU_CHECK(gpuEventRecord(ev1));
      GPU_CHECK(gpuEventSynchronize(ev1));
      float ms; GPU_CHECK(gpuEventElapsedTime(&ms, ev0, ev1));
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
  int max_nlev = 0;
  for (int ni = 0; ni < N_NLEVS; ni++) if (NLEVS[ni] > max_nlev) max_nlev = NLEVS[ni];
  size_t sz_max = (size_t)N_e * (max_nlev + 1);
  GPU_CHECK(gpuMalloc(&d_vnie, sz_max*8));
  GPU_CHECK(gpuMalloc(&d_zvt,  sz_max*8));
  GPU_CHECK(gpuMalloc(&d_zk,   sz_max*8));
  GPU_CHECK(gpuMalloc(&d_vn,   sz_max*8));
  GPU_CHECK(gpuMalloc(&d_vt,   sz_max*8));
  GPU_CHECK(gpuMalloc(&d_ubc, (size_t)N_e*2*8));
  GPU_CHECK(gpuMalloc(&d_wgt, (size_t)N_e*3*8));
  std::vector<double> h_ref_a(sz_max), h_ref_b(sz_max), h_ref_c(sz_max), h_gpu_out(sz_max);
  gpuEvent_t ev0, ev1; GPU_CHECK(gpuEventCreate(&ev0)); GPU_CHECK(gpuEventCreate(&ev1));

  const char *dists[3] = {"uniform", "normal_var1", "exact"};
  int ndists = have_exact ? 3 : 2;

  for (int ni = 0; ni < N_NLEVS; ni++) {
    int nlev = NLEVS[ni];
    size_t sz = (size_t)N_e * (nlev + 1);

    for (int di = 0; di < ndists; di++) {
      for (int V = 1; V <= 4; V++) {
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant(V);
        GPU_CHECK(gpuMemcpy(d_vnie, bd.h_vnie, sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_zvt,  bd.h_zvt,  sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_zk,   bd.h_zk,   sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_vn,   bd.h_vn,   sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_vt,   bd.h_vt,   sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_ubc,  bd.h_ubc, (size_t)N_e*2*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_wgt,  bd.h_wgt, (size_t)N_e*3*8, gpuMemcpyHostToDevice));
        run_configs_unblocked(fcsv, V, bd, dists[di],
                              d_vnie, d_zvt, d_zk, d_vn, d_vt, d_ubc, d_wgt, ev0, ev1,
                              h_ref_a.data(), h_ref_b.data(), h_ref_c.data(), h_gpu_out.data());
        fflush(fcsv); bd.free_all();
      }
    }

    for (int di = 0; di < ndists; di++) {
      for (int bi = 0; bi < N_BLOCK_SIZES; bi++) {
        int B = BLOCK_SIZES[bi];
        if (N_e % B != 0) { printf("SKIP GPU B=%d\n", B); continue; }
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant_blocked(B);
        GPU_CHECK(gpuMemcpy(d_vnie, bd.h_vnie, sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_zvt,  bd.h_zvt,  sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_zk,   bd.h_zk,   sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_vn,   bd.h_vn,   sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_vt,   bd.h_vt,   sz*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_ubc,  bd.h_ubc, (size_t)N_e*2*8, gpuMemcpyHostToDevice));
        GPU_CHECK(gpuMemcpy(d_wgt,  bd.h_wgt, (size_t)N_e*3*8, gpuMemcpyHostToDevice));
        run_configs_blocked(fcsv, bi, bd, dists[di],
                            d_vnie, d_zvt, d_zk, d_vn, d_vt, d_ubc, d_wgt, ev0, ev1,
                            h_ref_a.data(), h_ref_b.data(), h_ref_c.data(), h_gpu_out.data());
        fflush(fcsv); bd.free_all();
      }
    }
  }

  GPU_CHECK(gpuFree(d_vnie)); GPU_CHECK(gpuFree(d_zvt)); GPU_CHECK(gpuFree(d_zk));
  GPU_CHECK(gpuFree(d_vn));   GPU_CHECK(gpuFree(d_vt));  GPU_CHECK(gpuFree(d_ubc)); GPU_CHECK(gpuFree(d_wgt));
  GPU_CHECK(gpuEventDestroy(ev0)); GPU_CHECK(gpuEventDestroy(ev1));
  if (have_exact) ied.free_all();
  fclose(fcsv);
  return 0;
}
