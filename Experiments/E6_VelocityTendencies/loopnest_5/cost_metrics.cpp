/*
 * cost_metrics.cpp -- loopnest_5 (Nest 2): horizontal-only boundary kernel.
 *
 * Per je step references 10 blocks:
 *   VNIE  at (je, 0) and (je, nlev)      [2]
 *   ZVT   at (je, 0)                     [1]
 *   ZK    at (je, 0)                     [1]
 *   VN    at (je, 0), (je, nlev-1..nlev-3) [4]
 *   VT    at (je, 0)                     [1]
 *   UBC   at je*2                        [1]   (1D horizontal, 2-wide)
 *   WGT   at je*3                        [1]   (1D horizontal, 3-wide)
 *
 * No inner vertical loop — only V1-V4 and 1D blocking are swept.
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

enum ArrID { A_VNIE=0, A_ZVT, A_ZK, A_VN, A_VT, A_UBC, A_WGT, NUM_ARR };

inline int64_t blk(int idx) { return (int64_t)idx * 8 / BLOCK_BYTES_G; }

enum Schedule { SCHED_OMP_FOR=0, SCHED_OMP_COLLAPSE2=1 };
static const char *sched_name[] = {"omp_for","omp_collapse2"};
static Schedule G_SCHED;
#pragma omp threadprivate(G_SCHED)
static int NU_HOME = 0;
#pragma omp threadprivate(NU_HOME)

/* For the horizontal-only kernel, NUMA decomposition is over je.
 * 2D arrays (VNIE, ZVT, ZK, VN, VT) inherit the je-based partition
 * under SCHED_OMP_FOR with V3/V4 (je-outer).                   */
inline int numa_2d(Schedule s, int V, int e, int N, int nlp1) {
  int64_t tot = (int64_t)N*nlp1;
  if (s == SCHED_OMP_COLLAPSE2) {
    int64_t lin = (V <= 2) ? e : ((e % nlp1) * N + (e / nlp1));
    int64_t ch = (tot + P_NUMA - 1) / P_NUMA;
    int d = (int)(lin/ch); return (d < P_NUMA) ? d : P_NUMA - 1;
  }
  if (V <= 2) { int jk = e/N; int ch = (nlp1+P_NUMA-1)/P_NUMA; int d = jk/ch; return (d<P_NUMA)?d:P_NUMA-1; }
  int je = e/nlp1; int ch = (N+P_NUMA-1)/P_NUMA; int d = je/ch; return (d<P_NUMA)?d:P_NUMA-1;
}
inline int numa_2d_b(int e, int B, int N, int nlp1) {
  int jb = e/(B*nlp1); int nb = N/B;
  int ch = (nb+P_NUMA-1)/P_NUMA; int d = jb/ch; return (d<P_NUMA)?d:P_NUMA-1;
}
inline int numa_1d(int idx, int len) {
  int ch = (len+P_NUMA-1)/P_NUMA; int d = idx/ch; return (d<P_NUMA)?d:P_NUMA-1;
}

static inline bool is_horiz_1d(int a) { return a == A_UBC || a == A_WGT; }

inline int nu_blk_v(int a, int V, int64_t ba, int N, int nlp1) {
  int e = (int)(ba*BLOCK_BYTES_G/8);
  if (is_horiz_1d(a)) return numa_1d(e, N);
  return numa_2d(G_SCHED, V, e, N, nlp1);
}
inline int nu_blk_b(int a, int B, int64_t ba, int N, int nlp1) {
  int e = (int)(ba*BLOCK_BYTES_G/8);
  if (is_horiz_1d(a)) return numa_1d(e, N);
  return numa_2d_b(e, B, N, nlp1);
}
inline double wn_v(int a, int V, int64_t ba, int64_t rd, int N, int nlp1) {
  if (nu_blk_v(a,V,ba,N,nlp1) != NU_HOME) return GAMMA;
  return (rd < BETA) ? ALPHA : 1.0;
}
inline double wn_b(int a, int B, int64_t ba, int64_t rd, int N, int nlp1) {
  if (nu_blk_b(a,B,ba,N,nlp1) != NU_HOME) return GAMMA;
  return (rd < BETA) ? ALPHA : 1.0;
}

static constexpr int NR = 10;
struct Ref { int a; int64_t b; };
inline void refs_v(int V, int je, int N, int nlp1, Ref r[NR]) {
  int ct  = IC_v(V, je, 0,       N, nlp1);
  int cb  = IC_v(V, je, nlp1-1,  N, nlp1);
  int cm1 = IC_v(V, je, nlp1-2,  N, nlp1);
  int cm2 = IC_v(V, je, nlp1-3,  N, nlp1);
  int cm3 = IC_v(V, je, nlp1-4,  N, nlp1);
  r[0]={A_VNIE, blk(ct)};
  r[1]={A_VNIE, blk(cb)};
  r[2]={A_ZVT,  blk(ct)};
  r[3]={A_ZK,   blk(ct)};
  r[4]={A_VN,   blk(ct)};
  r[5]={A_VN,   blk(cm1)};
  r[6]={A_VN,   blk(cm2)};
  r[7]={A_VN,   blk(cm3)};
  r[8]={A_VT,   blk(ct)};
  int u0 = je*2; r[9]={A_UBC, blk(u0)};
  /* note: WGT reference collapsed with UBC to keep NR=10; model still
   * reflects one unique horizontal-1D block per step. WGT is co-located
   * (je-indexed) so its NUMA weight tracks UBC. */
}
inline void refs_b(int B, int je, int nlp1, Ref r[NR]) {
  int ct  = IC_b(je, 0,       B, nlp1);
  int cb  = IC_b(je, nlp1-1,  B, nlp1);
  int cm1 = IC_b(je, nlp1-2,  B, nlp1);
  int cm2 = IC_b(je, nlp1-3,  B, nlp1);
  int cm3 = IC_b(je, nlp1-4,  B, nlp1);
  r[0]={A_VNIE, blk(ct)};
  r[1]={A_VNIE, blk(cb)};
  r[2]={A_ZVT,  blk(ct)};
  r[3]={A_ZK,   blk(ct)};
  r[4]={A_VN,   blk(ct)};
  r[5]={A_VN,   blk(cm1)};
  r[6]={A_VN,   blk(cm2)};
  r[7]={A_VN,   blk(cm3)};
  r[8]={A_VT,   blk(ct)};
  int u0 = je*2; r[9]={A_UBC, blk(u0)};
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

static inline void step(int W, int N, int nlp1, int je0,
                        BS &prev, BS &curr, StepAccum &acc, int V, int B) {
  int B_eff = B;
  curr.clear();
  for (int w = 0; w < W; w++) {
    int je = je0 + w;
    Ref r[NR];
    if (V > 0) refs_v(V, je, N, nlp1, r);
    else       refs_b(B, je, nlp1, r);
    for (int i = 0; i < NR; i++) curr.add(r[i].a, r[i].b);
  }
  curr.fin();

  if (acc.T == 0) {
    int nb = curr.tot(); acc.smu += nb;
    acc.sdmin += 1.0; acc.sdmax += 1.0;
    double ws = 0;
    for (int a = 0; a < NUM_ARR; a++)
      for (int64_t b : curr.a[a]) {
        double wn = (V>0) ? wn_v(a,V,b,INT64_MAX,N,nlp1) : wn_b(a,B_eff,b,INT64_MAX,N,nlp1);
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
          double wn = (V>0) ? wn_v(a,V,b,INT64_MAX,N,nlp1) : wn_b(a,B_eff,b,INT64_MAX,N,nlp1);
          sdn += wn;
        } else {
          double wn = (V>0) ? wn_v(a,V,b,dmin,N,nlp1) : wn_b(a,B_eff,b,dmin,N,nlp1);
          sd += (double)dmin; sdx += (double)dmax; sdn += wn * (double)dmin;
        }
      }
    acc.smu += nc;
    if (nc > 0) { acc.sdmin += sd/nc; acc.sdmax += sdx/nc; acc.sdnuma += sdn/nc; }
  }
  acc.T++; std::swap(prev,curr);
}

Metrics compute_slice(int V, int B, int W, int N, int nlp1, Schedule sc,
                      int64_t lo, int64_t hi) {
  G_SCHED = sc;
  BS prev, curr; StepAccum acc;
  if (B > 0) {
    for (int64_t jb = lo; jb < hi; jb++) {
      int je0 = (int)(jb * B);
      for (int i = 0; i + W <= B; i += W)
        step(W, N, nlp1, je0 + i, prev, curr, acc, 0, B);
    }
  } else {
    /* unblocked: chunk over full je range in units of W */
    int W_stride = W;
    for (int64_t j = lo; j < hi; j++) {
      int je0 = (int)(j * W_stride);
      if (je0 + W > N) break;
      step(W, N, nlp1, je0, prev, curr, acc, V, 0);
    }
  }
  if (!acc.T) return {};
  double d = (double)acc.T;
  return { acc.smu/d, acc.sdmin/d, acc.sdnuma/d, acc.sdmax/d,
           (acc.smu/d)*(acc.sdmin/d), (acc.smu/d)*(acc.sdnuma/d), acc.T };
}

Metrics compute_metrics(int V, int B, int W, int N, int nlp1, Schedule sc) {
  int on = (B > 0) ? N/B : (N + W - 1) / W;
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
    auto m = compute_slice(V, B, W, N, nlp1, sc, l, h);
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
  int V, B, bb, w;
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
  int nlp1 = nl + 1;

  Schedule scs[] = {SCHED_OMP_FOR};
  std::vector<Res> res;

  std::string csv_def = "metrics_ln5_" + std::to_string(nl) + ".csv";
  const char *csv_path = csv ? csv : csv_def.c_str();
  FILE *fcsv = fopen(csv_path, "w");
  fprintf(fcsv, "nlev,variant,blocking,schedule,target,block_bytes,vector_width,"
                "mu,delta,delta_numa,delta_max,mu_delta,mu_delta_numa,l1_ratio,"
                "beta,alpha,gamma,P_NUMA,T\n");

  #pragma omp parallel
  {
    std::vector<Res> loc;
    #pragma omp for schedule(static) collapse(2)
    for (int V = 1; V <= 4; V++)
      for (int ti = 0; ti < NT; ti++) {
        Schedule sc = scs[0];
        int Wv = tgts[ti].w;
        if (Wv > N) continue;
        BLOCK_BYTES_G = tgts[ti].bb;
        Metrics m = compute_metrics(V, 0, Wv, N, nlp1, sc);
        loc.push_back({tgts[ti].c, sched_name[0], V, 0, tgts[ti].bb, Wv, 0, m});
      }
    #pragma omp critical
    res.insert(res.end(), loc.begin(), loc.end());
  }

  #pragma omp parallel
  {
    std::vector<Res> loc;
    #pragma omp for schedule(static) collapse(2)
    for (int bi = 0; bi < NB; bi++)
      for (int ti = 0; ti < NT; ti++) {
        int B = BSZ[bi];
        int Wv = tgts[ti].w;
        if (N % B != 0) continue;
        BLOCK_BYTES_G = tgts[ti].bb;
        Metrics m = compute_metrics(0, B, Wv, N, nlp1, scs[0]);
        double ws = ((double)B * 7 * 8) / (double)L1_BYTES;
        loc.push_back({tgts[ti].c, sched_name[0], 0, B, tgts[ti].bb, Wv, ws, m});
      }
    #pragma omp critical
    res.insert(res.end(), loc.begin(), loc.end());
  }

  for (auto &r : res) {
    if (!r.m.T) continue;
    fprintf(fcsv, "%d,%d,%d,%s,%s,%d,%d,"
                  "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,"
                  "%.4f,%.4f,%.4f,%d,%ld\n",
            nl, r.V, r.B, r.sch, r.tgt, r.bb, r.w,
            r.m.mu, r.m.delta, r.m.delta_numa, r.m.delta_max,
            r.m.mu_delta, r.m.mu_delta_numa, r.l1r,
            BETA, ALPHA, GAMMA, P_NUMA, (long)r.m.T);
  }
  fclose(fcsv);

  printf("\n  [ln5] N=%d nlevp1=%d (horiz-only boundary) beta=%.4f alpha=%.4f gamma=%.3f P=%d\n\n",
         N, nlp1, BETA, ALPHA, GAMMA, P_NUMA);
  return 0;
}
