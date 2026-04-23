/*
 * cost_metrics.cpp -- loopnest_3 (Nest 7): direct stencil, full vertical.
 *   z_v_grad_w = z_v_grad_w*gradh(jk)
 *              + vn_ie*(vn_ie*invr(jk) - ft_e(je))
 *              + z_vt_ie*(z_vt_ie*invr(jk) + fn_e(je))
 *
 * 7 arrays: 3 same-shape edge (W, VIE, VTI), 2 vertical-only 1D
 * (GH, INVR), 2 horizontal-only 1D (FT, FN). Full vertical jk in [0,nl).
 */
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <random>
#include <vector>

#include "../loopnest_1/icon_data_loader.h"

static double BETA = 1.0;
static double ALPHA = 0.012, GAMMA = 1.8;
static int P_NUMA = 4, BLOCK_BYTES_G = 64, L1_BYTES = 32768;
#pragma omp threadprivate(BLOCK_BYTES_G)

inline int IC_v(int V, int je, int jk, int N, int nl) {
  return (V <= 2) ? je + jk*N : jk + je*nl;
}
inline int IC_b(int je, int jk, int B, int nl) {
  return (je%B) + jk*B + (je/B)*B*nl;
}
inline int IC_t(int je, int jk, int TX, int TY, int nl) {
  int n_y = (nl + TY - 1) / TY;
  int xt = je/TX, yt = jk/TY;
  int xi = je - xt*TX, yi = jk - yt*TY;
  return xt*(n_y*TY*TX) + yt*(TY*TX) + yi*TX + xi;
}

enum ArrID { A_W=0, A_VIE, A_VTI, A_GH, A_INVR, A_FT, A_FN, NUM_ARR };
static const int eb[] = {8,8,8,8,8,8,8};

inline int64_t blk(int idx, int /*a*/) {
  return (int64_t)idx * 8 / BLOCK_BYTES_G;
}

enum Schedule { SCHED_OMP_FOR=0, SCHED_OMP_COLLAPSE2=1 };
static const char *sched_name[] = {"omp_for","omp_collapse2"};
enum LoopOrder { KLON_FIRST=0, KLEV_FIRST=1 };
inline LoopOrder iter_order(Schedule s, int V) {
  if (s == SCHED_OMP_COLLAPSE2) return KLON_FIRST;
  return (V <= 2) ? KLON_FIRST : KLEV_FIRST;
}
static Schedule G_SCHED;
#pragma omp threadprivate(G_SCHED)
static int NU_HOME = 0;
#pragma omp threadprivate(NU_HOME)

inline int numa_2d(Schedule s, int V, int e, int N, int nl) {
  int64_t tot = (int64_t)N*nl;
  if (s == SCHED_OMP_COLLAPSE2) {
    int64_t lin = (V <= 2) ? e : ((e % nl) * N + (e / nl));
    int64_t ch = (tot + P_NUMA - 1) / P_NUMA;
    int d = (int)(lin/ch); return (d < P_NUMA) ? d : P_NUMA - 1;
  }
  if (V <= 2) { int jk = e/N; int ch = (nl+P_NUMA-1)/P_NUMA; int d = jk/ch; return (d<P_NUMA)?d:P_NUMA-1; }
  int je = e/nl; int ch = (N+P_NUMA-1)/P_NUMA; int d = je/ch; return (d<P_NUMA)?d:P_NUMA-1;
}
inline int numa_2d_b(int e, int B, int N, int nl) {
  int jb = e/(B*nl); int nb = N/B;
  int ch = (nb+P_NUMA-1)/P_NUMA; int d = jb/ch; return (d<P_NUMA)?d:P_NUMA-1;
}
/* For 1D arrays, NUMA assignment follows 1D partition. */
inline int numa_1d(int idx, int len) {
  int ch = (len+P_NUMA-1)/P_NUMA; int d = idx/ch; return (d<P_NUMA)?d:P_NUMA-1;
}

inline int nu_blk_v(int a, int V, int64_t ba, int N, int nl) {
  int e = (int)(ba*BLOCK_BYTES_G/8);
  if (a == A_GH || a == A_INVR) return numa_1d(e, nl);
  if (a == A_FT || a == A_FN)   return numa_1d(e, N);
  return numa_2d(G_SCHED, V, e, N, nl);
}
inline int nu_blk_b(int a, int B, int64_t ba, int N, int nl) {
  int e = (int)(ba*BLOCK_BYTES_G/8);
  if (a == A_GH || a == A_INVR) return numa_1d(e, nl);
  if (a == A_FT || a == A_FN)   return numa_1d(e, N);
  return numa_2d_b(e, B, N, nl);
}
inline double wn_v(int a, int V, int64_t ba, int64_t rd, int N, int nl) {
  if (nu_blk_v(a,V,ba,N,nl) != NU_HOME) return GAMMA;
  return (rd < BETA) ? ALPHA : 1.0;
}
inline double wn_b(int a, int B, int64_t ba, int64_t rd, int N, int nl) {
  if (nu_blk_b(a,B,ba,N,nl) != NU_HOME) return GAMMA;
  return (rd < BETA) ? ALPHA : 1.0;
}

static constexpr int NR = 7;
struct Ref { int a; int64_t b; };
inline void refs_v(int V, int je, int jk, int N, int nl, Ref r[NR]) {
  int c = IC_v(V, je, jk, N, nl);
  r[0]={A_W,   blk(c,A_W)};
  r[1]={A_VIE, blk(c,A_VIE)};
  r[2]={A_VTI, blk(c,A_VTI)};
  r[3]={A_GH,  blk(jk,A_GH)};
  r[4]={A_INVR,blk(jk,A_INVR)};
  r[5]={A_FT,  blk(je,A_FT)};
  r[6]={A_FN,  blk(je,A_FN)};
}
inline void refs_b(int B, int je, int jk, int nl, Ref r[NR]) {
  int c = IC_b(je, jk, B, nl);
  r[0]={A_W,   blk(c,A_W)};
  r[1]={A_VIE, blk(c,A_VIE)};
  r[2]={A_VTI, blk(c,A_VTI)};
  r[3]={A_GH,  blk(jk,A_GH)};
  r[4]={A_INVR,blk(jk,A_INVR)};
  r[5]={A_FT,  blk(je,A_FT)};
  r[6]={A_FN,  blk(je,A_FN)};
}
inline void refs_t(int TX, int TY, int je, int jk, int nl, Ref r[NR]) {
  int c = IC_t(je, jk, TX, TY, nl);
  r[0]={A_W,   blk(c,A_W)};
  r[1]={A_VIE, blk(c,A_VIE)};
  r[2]={A_VTI, blk(c,A_VTI)};
  r[3]={A_GH,  blk(jk,A_GH)};
  r[4]={A_INVR,blk(jk,A_INVR)};
  r[5]={A_FT,  blk(je,A_FT)};
  r[6]={A_FN,  blk(je,A_FN)};
}

struct BS {
  std::vector<int64_t> a[NUM_ARR];
  void clear() { for (int i=0;i<NUM_ARR;i++) a[i].clear(); }
  void add(int ar, int64_t b) { a[ar].push_back(b); }
  void fin() { for(int i=0;i<NUM_ARR;i++){auto&v=a[i]; std::sort(v.begin(),v.end()); v.erase(std::unique(v.begin(),v.end()),v.end());} }
  int tot() const { int n=0; for(int i=0;i<NUM_ARR;i++) n+=(int)a[i].size(); return n; }
  bool has(int ar, int64_t b) const { return std::binary_search(a[ar].begin(),a[ar].end(),b); }
};

struct Metrics { double mu, delta, delta_numa, delta_max, mu_delta, mu_delta_numa; int64_t T; };
struct StepAccum { double smu=0, sdmin=0, sdnuma=0, sdmax=0; int64_t T=0; };

static inline void step(int W, int N, int nl, LoopOrder lo,
                        int je0, int jk0,
                        BS &prev, BS &curr, StepAccum &acc,
                        int V, int B, int TX=0, int TY=0) {
  int B_eff = (TY > 0) ? TX : B;
  curr.clear();
  for (int w = 0; w < W; w++) {
    int je = (lo==KLON_FIRST) ? je0+w : je0;
    int jk = (lo==KLON_FIRST) ? jk0   : jk0+w;
    Ref r[NR];
    if (TY > 0)     refs_t(TX, TY, je, jk, nl, r);
    else if (V > 0) refs_v(V, je, jk, N, nl, r);
    else            refs_b(B, je, jk, nl, r);
    for (int i = 0; i < NR; i++) curr.add(r[i].a, r[i].b);
  }
  curr.fin();

  if (acc.T == 0) {
    int nb = curr.tot(); acc.smu += nb;
    acc.sdmin += 1.0; acc.sdmax += 1.0;
    double ws = 0;
    for (int a = 0; a < NUM_ARR; a++)
      for (int64_t b : curr.a[a]) {
        double wn = (V>0) ? wn_v(a,V,b,INT64_MAX,N,nl) : wn_b(a,B_eff,b,INT64_MAX,N,nl);
        ws += wn;
      }
    acc.sdnuma += (nb>0) ? ws/nb : 0.0;
  } else {
    int nc = 0; double sd=0, sdx=0, sdn=0;
    for (int a = 0; a < NUM_ARR; a++)
      for (int64_t b : curr.a[a]) {
        if (prev.has(a,b)) continue;
        nc++;
        int64_t dmin = INT64_MAX, dmax = 0;
        for (int64_t bp : prev.a[a]) { int64_t d = std::abs(b - bp); if (d<dmin) dmin=d; if (d>dmax) dmax=d; }
        if (dmin == INT64_MAX) {
          sd += 1.0; sdx += 1.0;
          double wn = (V>0) ? wn_v(a,V,b,INT64_MAX,N,nl) : wn_b(a,B_eff,b,INT64_MAX,N,nl);
          sdn += wn;
        } else {
          double wn = (V>0) ? wn_v(a,V,b,dmin,N,nl) : wn_b(a,B_eff,b,dmin,N,nl);
          sd += (double)dmin; sdx += (double)dmax; sdn += wn * (double)dmin;
        }
      }
    acc.smu += nc;
    if (nc > 0) { acc.sdmin += sd/nc; acc.sdmax += sdx/nc; acc.sdnuma += sdn/nc; }
  }
  acc.T++; std::swap(prev,curr);
}

Metrics compute_slice(int V, int B, int W, int N, int nl, Schedule sc,
                      int64_t lo, int64_t hi, int jk_lo, int jk_hi) {
  G_SCHED = sc;
  LoopOrder lp = (V>0) ? iter_order(sc, V) : KLON_FIRST;
  BS prev, curr; StepAccum acc;

  if (B > 0) {
    for (int64_t jb = lo; jb < hi; jb++)
      for (int jk = jk_lo; jk < jk_hi; jk++)
        step(B, N, nl, KLON_FIRST, (int)jb*B, jk, prev, curr, acc, 0, B);
  } else if (sc == SCHED_OMP_COLLAPSE2) {
    for (int64_t ln = lo; ln < hi; ln++) {
      int jk = (int)(ln / N); if (jk < jk_lo || jk >= jk_hi) continue;
      int je0 = (int)(ln % N);
      for (int j = je0; j + W <= N; j += W)
        step(W, N, nl, KLON_FIRST, j, jk, prev, curr, acc, V, 0);
    }
  } else {
    int inn = (lp == KLON_FIRST) ? N : nl;
    for (int64_t o = lo; o < hi; o++)
      for (int i = 0; i + W <= inn; i += W) {
        int je = (lp == KLON_FIRST) ? i : (int)o;
        int jk = (lp == KLON_FIRST) ? (int)o : i;
        if (jk < jk_lo || jk >= jk_hi) continue;
        step(W, N, nl, lp, je, jk, prev, curr, acc, V, 0);
      }
  }

  if (!acc.T) return {};
  double d = (double)acc.T;
  return { acc.smu/d, acc.sdmin/d, acc.sdnuma/d, acc.sdmax/d,
           (acc.smu/d)*(acc.sdmin/d), (acc.smu/d)*(acc.sdnuma/d), acc.T };
}

Metrics compute_metrics(int V, int B, int W, int N, int nl, Schedule sc) {
  int jk_lo = 0, jk_hi = nl;
  LoopOrder lp = (V>0) ? iter_order(sc, V) : KLON_FIRST;
  int on = (B > 0) ? N/B : ((lp == KLON_FIRST) ? nl : N);
  Metrics ac = {}; int nd = 0;
  auto add = [&](Metrics &m) {
    if (!m.T) return;
    ac.mu += m.mu; ac.delta += m.delta; ac.delta_numa += m.delta_numa;
    ac.delta_max += m.delta_max; ac.mu_delta += m.mu_delta;
    ac.mu_delta_numa += m.mu_delta_numa; ac.T += m.T; nd++;
  };
  int ch = (on + P_NUMA - 1) / P_NUMA;
  for (int d = 0; d < P_NUMA; d++) {
    int l = d*ch, h = std::min((d+1)*ch, on);
    if (l >= h) continue;
    NU_HOME = d;
    auto m = compute_slice(V, B, W, N, nl, sc, l, h, jk_lo, jk_hi);
    add(m);
  }
  if (!nd) return {};
  double dn = (double)nd;
  return { ac.mu/dn, ac.delta/dn, ac.delta_numa/dn, ac.delta_max/dn,
           ac.mu_delta/dn, ac.mu_delta_numa/dn, ac.T };
}

static Metrics compute_slice_tiled(int TX, int TY, int W, int N, int nl,
                                   int lo_xb, int hi_xb, int jk_lo, int jk_hi) {
  G_SCHED = SCHED_OMP_FOR;
  BS prev, curr; StepAccum acc;
  int n_y = (nl + TY - 1) / TY;
  for (int64_t xb = lo_xb; xb < hi_xb; xb++)
    for (int yb = 0; yb < n_y; yb++)
      for (int y_in = 0; y_in < TY; y_in++) {
        int jk = yb*TY + y_in;
        if (jk >= nl || jk < jk_lo || jk >= jk_hi) continue;
        for (int j = 0; j + W <= TX; j += W)
          step(W, N, nl, KLON_FIRST, (int)xb*TX + j, jk, prev, curr, acc, 0, 0, TX, TY);
      }
  if (!acc.T) return {};
  double d = (double)acc.T;
  return { acc.smu/d, acc.sdmin/d, acc.sdnuma/d, acc.sdmax/d,
           (acc.smu/d)*(acc.sdmin/d), (acc.smu/d)*(acc.sdnuma/d), acc.T };
}
static Metrics compute_metrics_tiled(int TX, int TY, int W, int N, int nl) {
  int jk_lo = 0, jk_hi = nl;
  int on = N/TX;
  Metrics ac = {}; int nd = 0;
  auto add = [&](Metrics &m) {
    if (!m.T) return;
    ac.mu += m.mu; ac.delta += m.delta; ac.delta_numa += m.delta_numa;
    ac.delta_max += m.delta_max; ac.mu_delta += m.mu_delta;
    ac.mu_delta_numa += m.mu_delta_numa; ac.T += m.T; nd++;
  };
  int ch = (on + P_NUMA - 1) / P_NUMA;
  for (int d = 0; d < P_NUMA; d++) {
    int l = d*ch, h = std::min((d+1)*ch, on); if (l >= h) continue;
    NU_HOME = d;
    auto m = compute_slice_tiled(TX, TY, W, N, nl, l, h, jk_lo, jk_hi);
    add(m);
  }
  if (!nd) return {};
  double dn = (double)nd;
  return { ac.mu/dn, ac.delta/dn, ac.delta_numa/dn, ac.delta_max/dn,
           ac.mu_delta/dn, ac.mu_delta_numa/dn, ac.T };
}

struct Tgt { const char *n, *c; int bb, w; };
static const Tgt tgts[] = {
  {"CPU_scalar","cpu_scalar",64,1}, {"CPU_AVX512","cpu_avx512",64,8},
  {"GPU_scalar","gpu_scalar",128,1},{"GPU_Warp32","gpu_warp32",128,32}
};
static constexpr int NT = 4;
static const int BSZ[] = {8,16,32,64,128};
static constexpr int NB = 5;

struct Res {
  const char *tgt,*sch;
  int V, B, bb, w, TY;
  double l1r; Metrics m;
};

int main(int argc, char **argv) {
  const char *csv = (argc > 1) ? argv[1] : nullptr;
  int N  = (argc > 2) ? (int)atof(argv[2]) : 81920;
  int nl = (argc > 3) ? (int)atof(argv[3]) : 90;
  BETA   = (argc > 4) ? atof(argv[4]) : 1.0;
  ALPHA  = (argc > 5) ? atof(argv[5]) : 0.012;
  GAMMA  = (argc > 6) ? atof(argv[6]) : 1.8;
  P_NUMA = (argc > 7) ? (int)atof(argv[7]) : 4;
  L1_BYTES = (argc > 8) ? (int)atof(argv[8]) : 32768;

  /* Guard: atoi("0.25") returns 0, which makes every (x / P_NUMA) div */
  /* in this file a SIGFPE. Be defensive against run-script arg-order */
  /* mishaps and fall back to sane defaults instead of crashing.       */
  if (P_NUMA < 1) { fprintf(stderr, "[cost_metrics] argv[7] P_NUMA must be >= 1 (got %s); using 4\n", argc > 7 ? argv[7] : "<unset>"); P_NUMA = 4; }
  if (BETA < 0.0) { fprintf(stderr, "[cost_metrics] argv[4] BETA must be >= 0 (got %s); using 1.0\n", argc > 4 ? argv[4] : "<unset>"); BETA = 1.0; }
  if (L1_BYTES < 1) { fprintf(stderr, "[cost_metrics] argv[8] L1_BYTES must be >= 1 (got %s); using 32768\n", argc > 8 ? argv[8] : "<unset>"); L1_BYTES = 32768; }

  Schedule scs[] = {SCHED_OMP_FOR, SCHED_OMP_COLLAPSE2};
  std::vector<Res> res;

  std::string csv_def = "metrics_ln3_" + std::to_string(nl) + ".csv";
  const char *csv_path = csv ? csv : csv_def.c_str();
  FILE *fcsv = fopen(csv_path, "w");
  fprintf(fcsv, "nlev,variant,blocking,tile_y,schedule,target,block_bytes,vector_width,"
                "mu,delta,delta_numa,delta_max,mu_delta,mu_delta_numa,l1_ratio,"
                "beta,alpha,gamma,P_NUMA,T\n");

  #pragma omp parallel
  {
    std::vector<Res> loc;
    #pragma omp for schedule(static) collapse(3)
    for (int V = 1; V <= 4; V++)
      for (int si = 0; si < 2; si++)
        for (int ti = 0; ti < NT; ti++) {
          Schedule sc = scs[si];
          LoopOrder lp = iter_order(sc, V);
          int Wv = tgts[ti].w;
          int inn = (lp == KLON_FIRST) ? N : nl;
          if (Wv > inn) continue;
          BLOCK_BYTES_G = tgts[ti].bb;
          Metrics m = compute_metrics(V, 0, Wv, N, nl, sc);
          loc.push_back({tgts[ti].c, sched_name[si], V, 0, tgts[ti].bb, Wv, 0, 0, m});
        }
    #pragma omp critical
    res.insert(res.end(), loc.begin(), loc.end());
  }

  #pragma omp parallel
  {
    std::vector<Res> loc;
    #pragma omp for schedule(static) collapse(3)
    for (int bi = 0; bi < NB; bi++)
      for (int si = 0; si < 2; si++)
        for (int ti = 0; ti < NT; ti++) {
          int B = BSZ[bi]; Schedule sc = scs[si];
          int Wv = tgts[ti].w;
          if (N % B != 0) continue;
          BLOCK_BYTES_G = tgts[ti].bb;
          Metrics m = compute_metrics(0, B, Wv, N, nl, sc);
          double ws = ((double)B * 7 * 8) / (double)L1_BYTES;
          loc.push_back({tgts[ti].c, sched_name[si], 0, B, tgts[ti].bb, Wv, 0, ws, m});
        }
    #pragma omp critical
    res.insert(res.end(), loc.begin(), loc.end());
  }

  static const int TX_SZ[] = {8,16,32,64};
  static const int TY_SZ[] = {8,16,32,64};
  #pragma omp parallel
  {
    std::vector<Res> loc;
    #pragma omp for schedule(static) collapse(3)
    for (int txi = 0; txi < 4; txi++)
      for (int tyi = 0; tyi < 4; tyi++)
        for (int ti = 0; ti < NT; ti++) {
          int TX = TX_SZ[txi], TY = TY_SZ[tyi];
          int Wv = tgts[ti].w;
          if (N % TX != 0) continue;
          if (nl % TY != 0) continue;
          if (Wv > TX) continue;
          BLOCK_BYTES_G = tgts[ti].bb;
          Metrics m = compute_metrics_tiled(TX, TY, Wv, N, nl);
          double ws = ((double)TX * 7 * 8) / (double)L1_BYTES;
          loc.push_back({tgts[ti].c, "tiled_omp_for", 0, TX, tgts[ti].bb, Wv, TY, ws, m});
        }
    #pragma omp critical
    res.insert(res.end(), loc.begin(), loc.end());
  }

  for (auto &r : res) {
    if (!r.m.T) continue;
    fprintf(fcsv, "%d,%d,%d,%d,%s,%s,%d,%d,"
                  "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,"
                  "%.4f,%.4f,%.4f,%d,%ld\n",
            nl, r.V, r.B, r.TY, r.sch, r.tgt, r.bb, r.w,
            r.m.mu, r.m.delta, r.m.delta_numa, r.m.delta_max,
            r.m.mu_delta, r.m.mu_delta_numa, r.l1r,
            BETA, ALPHA, GAMMA, P_NUMA, (long)r.m.T);
  }
  fclose(fcsv);

  printf("\n  [ln3] N=%d nl=%d (full vert) beta=%.4f alpha=%.4f gamma=%.3f P=%d\n\n",
         N, nl, BETA, ALPHA, GAMMA, P_NUMA);
  return 0;
}
