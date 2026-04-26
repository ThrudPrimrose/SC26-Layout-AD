/*
 * bench_cpu.cpp -- loopnest_4 (Nest 24) CPU benchmark.
 * Indirect stencil + CFL-gated diffusion, partial vertical jk in [jk_lo, jk_hi).
 *
 *   if (lvlm[jk] || lvlm[jk+1]) {
 *     for each je:
 *       w_ce = cle[je*2+0]*zw[IC(ici[je*2+0],jk)] + cle[je*2+1]*zw[IC(ici[je*2+1],jk)]
 *       if (|w_ce| > CFL_W*ddqz[IC(je,jk)]) {
 *         ... gf-indirect sum over iqi + tang*invp*(zeta diff over ivi) ...
 *         ddt[IC(je,jk)] += difcoef * area[je] * sum
 *       }
 *   }
 */
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <omp.h>
#include <vector>

#include "bench_common.h"
#include "../loopnest_1/icon_data_loader.h"
#include "../../common/jacobi_flush.h"

using clk = std::chrono::high_resolution_clock;

#define STENCIL_V(V) \
  int c  = IC<V>(je, jk, N_e, nlev); \
  int c0 = IC<V>(ici[je*2+0], jk, N_e, nlev); \
  int c1 = IC<V>(ici[je*2+1], jk, N_e, nlev); \
  double wce = cle[je*2+0]*zw[c0] + cle[je*2+1]*zw[c1]; \
  double dq  = ddqz[c]; \
  if (std::fabs(wce) > CFL_W_LIMIT * dq) { \
    double dif = SFEX * std::min(0.85 - CFL_W_LIMIT*DTIME, \
                                 std::fabs(wce)*DTIME/dq - CFL_W_LIMIT*DTIME); \
    double s = gf[je*5+0]*vn[c]; \
    s += gf[je*5+1]*vn[IC<V>(iqi[je*4+0], jk, N_e, nlev)]; \
    s += gf[je*5+2]*vn[IC<V>(iqi[je*4+1], jk, N_e, nlev)]; \
    s += gf[je*5+3]*vn[IC<V>(iqi[je*4+2], jk, N_e, nlev)]; \
    s += gf[je*5+4]*vn[IC<V>(iqi[je*4+3], jk, N_e, nlev)]; \
    s += tang[je]*invp[je]*(zeta[IC<V>(ivi[je*2+1], jk, N_e, nlev)] \
                           - zeta[IC<V>(ivi[je*2+0], jk, N_e, nlev)]); \
    ddt[c] += dif * area[je] * s; \
  }

#define STENCIL_B(B) \
  int c  = IC_blocked(je, jk, B, nlev); \
  int c0 = IC_blocked(ici[je*2+0], jk, B, nlev); \
  int c1 = IC_blocked(ici[je*2+1], jk, B, nlev); \
  double wce = cle[je*2+0]*zw[c0] + cle[je*2+1]*zw[c1]; \
  double dq  = ddqz[c]; \
  if (std::fabs(wce) > CFL_W_LIMIT * dq) { \
    double dif = SFEX * std::min(0.85 - CFL_W_LIMIT*DTIME, \
                                 std::fabs(wce)*DTIME/dq - CFL_W_LIMIT*DTIME); \
    double s = gf[je*5+0]*vn[c]; \
    s += gf[je*5+1]*vn[IC_blocked(iqi[je*4+0], jk, B, nlev)]; \
    s += gf[je*5+2]*vn[IC_blocked(iqi[je*4+1], jk, B, nlev)]; \
    s += gf[je*5+3]*vn[IC_blocked(iqi[je*4+2], jk, B, nlev)]; \
    s += gf[je*5+4]*vn[IC_blocked(iqi[je*4+3], jk, B, nlev)]; \
    s += tang[je]*invp[je]*(zeta[IC_blocked(ivi[je*2+1], jk, B, nlev)] \
                           - zeta[IC_blocked(ivi[je*2+0], jk, B, nlev)]); \
    ddt[c] += dif * area[je] * s; \
  }

#define STENCIL_T(TX, TY) \
  int c  = IC_tiled(je, jk, TX, TY, N_e, nlev); \
  int c0 = IC_tiled(ici[je*2+0], jk, TX, TY, N_e, nlev); \
  int c1 = IC_tiled(ici[je*2+1], jk, TX, TY, N_e, nlev); \
  double wce = cle[je*2+0]*zw[c0] + cle[je*2+1]*zw[c1]; \
  double dq  = ddqz[c]; \
  if (std::fabs(wce) > CFL_W_LIMIT * dq) { \
    double dif = SFEX * std::min(0.85 - CFL_W_LIMIT*DTIME, \
                                 std::fabs(wce)*DTIME/dq - CFL_W_LIMIT*DTIME); \
    double s = gf[je*5+0]*vn[c]; \
    s += gf[je*5+1]*vn[IC_tiled(iqi[je*4+0], jk, TX, TY, N_e, nlev)]; \
    s += gf[je*5+2]*vn[IC_tiled(iqi[je*4+1], jk, TX, TY, N_e, nlev)]; \
    s += gf[je*5+3]*vn[IC_tiled(iqi[je*4+2], jk, TX, TY, N_e, nlev)]; \
    s += gf[je*5+4]*vn[IC_tiled(iqi[je*4+3], jk, TX, TY, N_e, nlev)]; \
    s += tang[je]*invp[je]*(zeta[IC_tiled(ivi[je*2+1], jk, TX, TY, N_e, nlev)] \
                           - zeta[IC_tiled(ivi[je*2+0], jk, TX, TY, N_e, nlev)]); \
    ddt[c] += dif * area[je] * s; \
  }

#define KARGS \
  double *__restrict ddt, const double *__restrict vn, const double *__restrict zw, \
  const double *__restrict zeta, const double *__restrict ddqz, \
  const double *__restrict cle, const double *__restrict gf, \
  const double *__restrict area, const double *__restrict tang, const double *__restrict invp, \
  const int *__restrict ici, const int *__restrict iqi, const int *__restrict ivi, \
  const int *__restrict lvlm, \
  int N_e, int nlev, int jk_start, int jk_end

template <int V> static void cpu_unblocked_jk_outer(KARGS) {
#pragma omp parallel for schedule(static)
  for (int jk = jk_start; jk < jk_end; jk++) {
    if (!(lvlm[jk] || lvlm[jk+1])) continue;
    for (int je = 0; je < N_e; je++) { STENCIL_V(V) }
  }
}
template <int V> static void cpu_unblocked_je_outer(KARGS) {
#pragma omp parallel for schedule(static)
  for (int je = 0; je < N_e; je++)
    for (int jk = jk_start; jk < jk_end; jk++) {
      if (!(lvlm[jk] || lvlm[jk+1])) continue;
      STENCIL_V(V)
    }
}
template <int V> static void cpu_unblocked_collapse2(KARGS) {
#pragma omp parallel for collapse(2) schedule(static)
  for (int jk = jk_start; jk < jk_end; jk++)
    for (int je = 0; je < N_e; je++) {
      if (!(lvlm[jk] || lvlm[jk+1])) continue;
      STENCIL_V(V)
    }
}

template <int B> static void cpu_blocked_je_outer(KARGS) {
  int nblk = N_e / B;
#pragma omp parallel for schedule(static)
  for (int jb = 0; jb < nblk; jb++)
    for (int jk = jk_start; jk < jk_end; jk++) {
      if (!(lvlm[jk] || lvlm[jk+1])) continue;
      for (int jl = 0; jl < B; jl++) { int je = jb*B + jl; STENCIL_B(B) }
    }
}
template <int B> static void cpu_blocked_collapse2(KARGS) {
  int nblk = N_e / B;
#pragma omp parallel for collapse(2) schedule(static)
  for (int jb = 0; jb < nblk; jb++)
    for (int jk = jk_start; jk < jk_end; jk++) {
      if (!(lvlm[jk] || lvlm[jk+1])) continue;
      for (int jl = 0; jl < B; jl++) { int je = jb*B + jl; STENCIL_B(B) }
    }
}

template <int TX, int TY> static void cpu_tiled_je_outer(KARGS) {
  int nx = N_e / TX;
  int ny = (nlev + TY - 1) / TY;
#pragma omp parallel for schedule(static)
  for (int xb = 0; xb < nx; xb++)
    for (int yb = 0; yb < ny; yb++) {
      int jk0 = yb*TY, jk1 = std::min(jk0 + TY, nlev);
      for (int jk = jk0; jk < jk1; jk++) {
        if (jk < jk_start || jk >= jk_end) continue;
        if (!(lvlm[jk] || lvlm[jk+1])) continue;
        for (int xi = 0; xi < TX; xi++) { int je = xb*TX + xi; STENCIL_T(TX, TY) }
      }
    }
}
template <int TX, int TY> static void cpu_tiled_collapse2(KARGS) {
  int nx = N_e / TX;
  int ny = (nlev + TY - 1) / TY;
#pragma omp parallel for collapse(2) schedule(static)
  for (int xb = 0; xb < nx; xb++)
    for (int yb = 0; yb < ny; yb++) {
      int jk0 = yb*TY, jk1 = std::min(jk0 + TY, nlev);
      for (int jk = jk0; jk < jk1; jk++) {
        if (jk < jk_start || jk >= jk_end) continue;
        if (!(lvlm[jk] || lvlm[jk+1])) continue;
        for (int xi = 0; xi < TX; xi++) { int je = xb*TX + xi; STENCIL_T(TX, TY) }
      }
    }
}

using KFn = void (*)(double*, const double*, const double*, const double*, const double*,
                     const double*, const double*, const double*, const double*, const double*,
                     const int*, const int*, const int*, const int*,
                     int, int, int, int);

static KFn kfun_unblocked[4][3] = {
  {cpu_unblocked_jk_outer<1>, cpu_unblocked_je_outer<1>, cpu_unblocked_collapse2<1>},
  {cpu_unblocked_jk_outer<2>, cpu_unblocked_je_outer<2>, cpu_unblocked_collapse2<2>},
  {cpu_unblocked_jk_outer<3>, cpu_unblocked_je_outer<3>, cpu_unblocked_collapse2<3>},
  {cpu_unblocked_jk_outer<4>, cpu_unblocked_je_outer<4>, cpu_unblocked_collapse2<4>},
};
static KFn kfun_blocked[3][2] = {
  {cpu_blocked_je_outer<8>,   cpu_blocked_collapse2<8>  },
  {cpu_blocked_je_outer<16>,  cpu_blocked_collapse2<16> },
  {cpu_blocked_je_outer<32>,  cpu_blocked_collapse2<32> },
};
static KFn kfun_tiled_je[3][3] = {
  {cpu_tiled_je_outer<8,8>,  cpu_tiled_je_outer<8,16>,  cpu_tiled_je_outer<8,32> },
  {cpu_tiled_je_outer<16,8>, cpu_tiled_je_outer<16,16>, cpu_tiled_je_outer<16,32>},
  {cpu_tiled_je_outer<32,8>, cpu_tiled_je_outer<32,16>, cpu_tiled_je_outer<32,32>},
};
static KFn kfun_tiled_co[3][3] = {
  {cpu_tiled_collapse2<8,8>,  cpu_tiled_collapse2<8,16>,  cpu_tiled_collapse2<8,32> },
  {cpu_tiled_collapse2<16,8>, cpu_tiled_collapse2<16,16>, cpu_tiled_collapse2<16,32>},
  {cpu_tiled_collapse2<32,8>, cpu_tiled_collapse2<32,16>, cpu_tiled_collapse2<32,32>},
};

static inline double elapsed_ms(clk::time_point a, clk::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}

/* Canonical Jacobi-2D cache flush (see ../../common/jacobi_flush.h). */
static void jacobi_flush(double *, double *, int) { flush_jacobi(); }

static SchedKind sched_for(int V) { return (V <= 2) ? SCHED_JK_OUTER : SCHED_JE_OUTER; }

/* CALL_K is used by both the verify functions below and the timed
 * run_variant / run_blocked / run_tiled runners; define it once here. */
#define CALL_K(fn) \
  fn(bd.h_ddt, bd.h_vn, bd.h_zw, bd.h_zeta, bd.h_ddqz, \
     bd.h_cle, bd.h_gf, bd.h_area, bd.h_tang, bd.h_invp, \
     bd.h_ici, bd.h_iqi, bd.h_ivi, bd.h_lvlm, \
     bd.N_e, bd.nlev, jk0, jk1)

/* ================================================================== */
/*  Numerical verification                                             */
/*                                                                    */
/*  Strategy: compute a row-major (V=1) reference from the src_* input*/
/*  arrays and compare every layout variant's output (after a single  */
/*  kernel call) against it, re-reading through that variant's IC().  */
/*  The reference is independent of the kernel templates so it won't  */
/*  mask a systematic bug introduced into every template.             */
/*                                                                    */
/*  Tolerance: the kernel performs multiply-add fused ops in a well-  */
/*  defined order and bit-identical arithmetic on every layout (only  */
/*  the address computation changes). We therefore check with strict  */
/*  bitwise equality first, then fall back to a relative tolerance of */
/*  1e-12 to tolerate permitted FMA contraction differences.          */
/* ================================================================== */

static void ref_kernel_v1(
    double *__restrict__ ddt_rm, const double *__restrict__ vn_rm,
    const double *__restrict__ zw_rm, const double *__restrict__ zeta_rm,
    const double *__restrict__ ddqz_rm,
    const double *__restrict__ cle, const double *__restrict__ gf,
    const double *__restrict__ area, const double *__restrict__ tang,
    const double *__restrict__ invp,
    const int *__restrict__ ici, const int *__restrict__ iqi,
    const int *__restrict__ ivi, const int *__restrict__ lvlm,
    int N_e, int nlev, int jk_start, int jk_end)
{
  for (int jk = jk_start; jk < jk_end; jk++) {
    if (!(lvlm[jk] || lvlm[jk+1])) continue;
    for (int je = 0; je < N_e; je++) {
      int c  = je + jk * N_e;
      int c0 = ici[je*2+0] + jk * N_e;
      int c1 = ici[je*2+1] + jk * N_e;
      double wce = cle[je*2+0]*zw_rm[c0] + cle[je*2+1]*zw_rm[c1];
      double dq = ddqz_rm[c];
      if (std::fabs(wce) > CFL_W_LIMIT * dq) {
        double dif = SFEX * std::min(0.85 - CFL_W_LIMIT*DTIME,
                                     std::fabs(wce)*DTIME/dq - CFL_W_LIMIT*DTIME);
        double s = gf[je*5+0]*vn_rm[c];
        s += gf[je*5+1]*vn_rm[iqi[je*4+0] + jk*N_e];
        s += gf[je*5+2]*vn_rm[iqi[je*4+1] + jk*N_e];
        s += gf[je*5+3]*vn_rm[iqi[je*4+2] + jk*N_e];
        s += gf[je*5+4]*vn_rm[iqi[je*4+3] + jk*N_e];
        s += tang[je]*invp[je]*(zeta_rm[ivi[je*2+1] + jk*N_e]
                              - zeta_rm[ivi[je*2+0] + jk*N_e]);
        ddt_rm[c] += dif * area[je] * s;
      }
    }
  }
}

/* Compare `got` (read through `to_flat(je, jk)`) against `ref_rm`. */
template <typename Index>
static long long verify_compare(const double *got, const double *ref_rm,
                                int N_e, int nlev, int jk0, int jk1,
                                Index to_flat) {
  long long bad = 0;
  double maxrel = 0.0;
  #pragma omp parallel for schedule(static) reduction(+:bad) reduction(max:maxrel)
  for (int jk = jk0; jk < jk1; jk++)
    for (int je = 0; je < N_e; je++) {
      double g = got[to_flat(je, jk)];
      double r = ref_rm[je + jk * N_e];
      if (g == r) continue;
      double denom = std::fabs(r);
      double rel = denom > 0 ? std::fabs(g - r) / denom : std::fabs(g - r);
      if (rel > maxrel) maxrel = rel;
      if (rel > 1e-12) bad++;
    }
  if (bad) fprintf(stderr, "[verify][FAIL] loopnest_4 bad=%lld maxrel=%g\n", bad, maxrel);
  return bad;
}

/* Verify a single layout variant V. Caller must have set bd.set_variant(V, ...)
 * before the call; on return, bd.h_ddt has been overwritten by one kernel
 * execution and must be reset if further timing is intended. */
static void verify_variant_V(int V, BenchData &bd, int jk0, int jk1,
                             const double *ref_rm) {
  int ki = V - 1;
  CALL_K(kfun_unblocked[ki][0]);   /* jk_outer is the least-reorder reference */
  int N_e = bd.N_e, nlev = bd.nlev;
  long long bad = 0;
  switch (V) {
    case 1: bad = verify_compare(bd.h_ddt, ref_rm, N_e, nlev, jk0, jk1,
              [&](int je,int jk){ return IC<1>(je,jk,N_e,nlev); }); break;
    case 2: bad = verify_compare(bd.h_ddt, ref_rm, N_e, nlev, jk0, jk1,
              [&](int je,int jk){ return IC<2>(je,jk,N_e,nlev); }); break;
    case 3: bad = verify_compare(bd.h_ddt, ref_rm, N_e, nlev, jk0, jk1,
              [&](int je,int jk){ return IC<3>(je,jk,N_e,nlev); }); break;
    case 4: bad = verify_compare(bd.h_ddt, ref_rm, N_e, nlev, jk0, jk1,
              [&](int je,int jk){ return IC<4>(je,jk,N_e,nlev); }); break;
  }
  if (!bad) fprintf(stderr, "[verify][OK]   loopnest_4 V%d\n", V);
}

static void verify_blocked_B(int B, BenchData &bd, int jk0, int jk1,
                             const double *ref_rm) {
  int bi = -1;
  for (int i = 0; i < N_BLOCK_SIZES; i++) if (BLOCK_SIZES[i] == B) { bi = i; break; }
  if (bi < 0) return;
  CALL_K(kfun_blocked[bi][0]);
  int N_e = bd.N_e, nlev = bd.nlev;
  long long bad = verify_compare(bd.h_ddt, ref_rm, N_e, nlev, jk0, jk1,
      [&](int je,int jk){ return IC_blocked(je,jk,B,nlev); });
  if (!bad) fprintf(stderr, "[verify][OK]   loopnest_4 B=%d\n", B);
}

static void run_variant(FILE *fcsv, int V, BenchData &bd,
                        int jk0, int jk1, const char *dist,
                        double *fb0, double *fb1, int FN) {
  bd.set_variant(V, sched_for(V));
  int ki = V - 1;
  const char *sch_names[3] = {"jk_outer", "je_outer", "collapse2"};
  for (int si = 0; si < 3; si++) {
    for (int r = 0; r < WARMUP; r++) {
      CALL_K(kfun_unblocked[ki][si]);
      jacobi_flush(fb0, fb1, FN);
    }
    for (int r = 0; r < NRUNS; r++) {
      auto t0 = clk::now();
      CALL_K(kfun_unblocked[ki][si]);
      auto t1 = clk::now();
      fprintf(fcsv, "cpu,%d,%d,%d,%s,V%d,%s,%d,%.6f\n",
              V, bd.nlev, bd.N_e, dist, V, sch_names[si], r, elapsed_ms(t0,t1));
      jacobi_flush(fb0, fb1, FN);
    }
  }
}

static void run_blocked(FILE *fcsv, int bi, BenchData &bd,
                        int jk0, int jk1, const char *dist,
                        double *fb0, double *fb1, int FN) {
  int B = BLOCK_SIZES[bi];
  const char *sch_names[2] = {"omp_for", "collapse2"};
  for (int si = 0; si < 2; si++) {
    bd.change_schedule((si == 0) ? SCHED_JE_OUTER : SCHED_COLLAPSE2);
    for (int r = 0; r < WARMUP; r++) {
      CALL_K(kfun_blocked[bi][si]);
      jacobi_flush(fb0, fb1, FN);
    }
    for (int r = 0; r < NRUNS; r++) {
      auto t0 = clk::now();
      CALL_K(kfun_blocked[bi][si]);
      auto t1 = clk::now();
      fprintf(fcsv, "cpu,0,%d,%d,%s,B%d,blocked_%s,%d,%.6f\n",
              bd.nlev, bd.N_e, dist, B, sch_names[si], r, elapsed_ms(t0,t1));
      jacobi_flush(fb0, fb1, FN);
    }
  }
}

static void run_tiled(FILE *fcsv, int txi, int tyi, BenchData &bd,
                      int jk0, int jk1, const char *dist,
                      double *fb0, double *fb1, int FN) {
  int TX = TILE_X_VALUES[txi];
  int TY = TILE_Y_VALUES[tyi + 1];
  const char *sch_names[2] = {"omp_for", "collapse2"};
  for (int si = 0; si < 2; si++) {
    KFn k = (si == 0) ? kfun_tiled_je[txi][tyi] : kfun_tiled_co[txi][tyi];
    for (int r = 0; r < WARMUP; r++) {
      CALL_K(k);
      jacobi_flush(fb0, fb1, FN);
    }
    for (int r = 0; r < NRUNS; r++) {
      auto t0 = clk::now();
      CALL_K(k);
      auto t1 = clk::now();
      fprintf(fcsv, "cpu,0,%d,%d,%s,%d,tiled_%s_ty%d,%d,%.6f\n",
              bd.nlev, bd.N_e, dist, TX, sch_names[si], TY, r, elapsed_ms(t0,t1));
      jacobi_flush(fb0, fb1, FN);
    }
  }
}

int main(int argc, char *argv[]) {
  const char *csv_path = (argc >= 2) ? argv[1] : "ddt_vn_vert_cpu.csv";
  FILE *fcsv = fopen(csv_path, "w"); if (!fcsv) { perror("fopen"); return 1; }
  fprintf(fcsv, "backend,V,nlev,N_e,cell_dist,layout,schedule,run_id,time_ms\n");

  int step = (argc >= 3) ? atoi(argv[2]) : 9;
  std::string gp = icon_global_path(step), pp = icon_patch_path(step);
  int icon_nproma = icon_read_nproma(gp.c_str());
  IconEdgeData ied;
  bool have_exact = (icon_nproma > 0) && icon_load_patch(pp.c_str(), icon_nproma, ied);

  int N_e = have_exact ? ied.n_edges : 122880;
  /* Canonical flush owns its buffers (see ../../common/jacobi_flush.h). */
  double *fb0 = nullptr, *fb1 = nullptr;
  const int FN = 0;
  flush_jacobi_init();

  for (int ni = 0; ni < N_NLEVS; ni++) {
    int nlev = NLEVS[ni];
    int jk0 = jk_lo_for(nlev), jk1 = jk_hi_for(nlev);

    const char *dists[3] = {"uniform", "normal_var1", "exact"};
    int ndists = have_exact ? 3 : 2;

    for (int di = 0; di < ndists; di++) {
      BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);

      /* Reference: compute the row-major (V=1) expected output once.
       * The reference reads src_* directly and writes into `ref_rm`
       * which starts as a copy of src_ddt. */
      std::vector<double> ref_rm((size_t)N_e * nlev);
      std::memcpy(ref_rm.data(), bd.src_ddt, (size_t)N_e * nlev * sizeof(double));
      ref_kernel_v1(ref_rm.data(), bd.src_vn, bd.src_zw, bd.src_zeta, bd.src_ddqz,
                    bd.h_cle, bd.h_gf, bd.h_area, bd.h_tang, bd.h_invp,
                    bd.h_ici, bd.h_iqi, bd.h_ivi, bd.h_lvlm,
                    N_e, nlev, jk0, jk1);

      if (di == 0) {           /* verify only once per nlev to keep runtime bounded */
        for (int V = 1; V <= 4; V++) {
          bd.set_variant(V, sched_for(V));
          verify_variant_V(V, bd, jk0, jk1, ref_rm.data());
        }
      }

      for (int V = 1; V <= 4; V++) run_variant(fcsv, V, bd, jk0, jk1, dists[di], fb0, fb1, FN);
      fflush(fcsv); bd.free_all();
    }

    for (int di = 0; di < ndists; di++) {
      for (int bi = 0; bi < N_BLOCK_SIZES; bi++) {
        int B = BLOCK_SIZES[bi];
        if (N_e % B != 0) { printf("SKIP B=%d !| N_e=%d\n", B, N_e); continue; }
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant_blocked(B, SCHED_JE_OUTER);

        if (di == 0) {  /* one verification per (nlev, B) */
          std::vector<double> ref_rm((size_t)N_e * nlev);
          std::memcpy(ref_rm.data(), bd.src_ddt, (size_t)N_e * nlev * sizeof(double));
          ref_kernel_v1(ref_rm.data(), bd.src_vn, bd.src_zw, bd.src_zeta, bd.src_ddqz,
                        bd.h_cle, bd.h_gf, bd.h_area, bd.h_tang, bd.h_invp,
                        bd.h_ici, bd.h_iqi, bd.h_ivi, bd.h_lvlm,
                        N_e, nlev, jk0, jk1);
          verify_blocked_B(B, bd, jk0, jk1, ref_rm.data());
          bd.set_variant_blocked(B, SCHED_JE_OUTER);  /* reset h_ddt after verify */
        }

        run_blocked(fcsv, bi, bd, jk0, jk1, dists[di], fb0, fb1, FN);
        fflush(fcsv); bd.free_all();
      }
    }

    for (int di = 0; di < ndists; di++) {
      for (int tx_i = 0; tx_i < N_TILE_X; tx_i++) {
        int TX = TILE_X_VALUES[tx_i];
        if (N_e % TX != 0) continue;
        for (int ty_i = 0; ty_i < 3; ty_i++) {
          int TY = TILE_Y_VALUES[ty_i + 1];
          if (nlev % TY != 0) continue;
          BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
          bd.set_variant_tiled(TX, TY, SCHED_JE_OUTER);
          run_tiled(fcsv, tx_i, ty_i, bd, jk0, jk1, dists[di], fb0, fb1, FN);
          fflush(fcsv); bd.free_all();
        }
      }
    }
  }

  /* fb0/fb1 are nullptr; canonical flush owns its buffers. */
  if (have_exact) ied.free_all();
  fclose(fcsv);
  return 0;
}
