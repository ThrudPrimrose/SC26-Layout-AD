/*
 * cost_metrics.cpp -- loopnest_6 (Nest 25): vertical-only level reduction.
 *
 *   DO jk = MAX(3, nrdmax-2), nlev-3
 *     levelmask(jk) = ANY(levmask(i_startblk:i_endblk, jk))
 *   END DO
 *
 * Only one 2D array (levmask) carries layout sensitivity. The output
 * `levelmask` is a small 1D vertical vector and is NUMA-replicable, so
 * it is not modelled in the step references.
 *
 * Step pattern: outer jk over the partial vertical range; inner step
 * walks W consecutive je's at a single (je, jk). NR=1 reference per je
 * (read of levmask at (je, jk)).
 *
 * NUMA partitioning follows the kernel parallelism (jk-outer). V1/V2
 * (je-first) with a jk-outer schedule keeps row (*, jk) local; V3/V4
 * (jk-first) spreads each row across all NUMA domains.
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

static int BETA = 1;
static double ALPHA = 0.012, GAMMA = 1.8;
static int P_NUMA = 4, BLOCK_BYTES_G = 64, L1_BYTES = 32768;
#pragma omp threadprivate(BLOCK_BYTES_G)

inline int IC_v(int V, int je, int jk, int N, int nl) {
  return (V <= 2) ? je + jk*N : jk + je*nl;
}
inline int IC_b(int je, int jk, int B, int nl) {
  return (je%B) + jk*B + (je/B)*B*nl;
}
inline int IC_t(int je, int jk, int TX, int TY, int N, int nl) {
  int n_y = (nl + TY - 1) / TY;
  int xt = je / TX, yt = jk / TY;
  int xi = je - xt*TX, yi = jk - yt*TY;
  return xt*(n_y*TY*TX) + yt*(TY*TX) + yi*TX + xi;
}

enum ArrID { A_LVM=0, NUM_ARR };

inline int64_t blk(int idx) { return (int64_t)idx * 8 / BLOCK_BYTES_G; }

enum Schedule { SCHED_OMP_FOR=0, SCHED_OMP_COLLAPSE2=1 };
static const char *sched_name[] = {"omp_for","omp_collapse2"};
static Schedule G_SCHED;
#pragma omp threadprivate(G_SCHED)
static int NU_HOME = 0;
#pragma omp threadprivate(NU_HOME)

/* NUMA home for a memory block of levmask under jk-outer kernel sched:
 *   V1/V2 (je inner, jk outer in memory): row (*, jk) contiguous → partition by jk.
 *   V3/V4 (jk inner, je outer in memory): each je column contiguous → partition by je.
 */
inline int numa_2d(int V, int e, int N, int nl) {
  if (V <= 2) { int jk = e/N; int ch = (nl+P_NUMA-1)/P_NUMA; int d = jk/ch; return (d<P_NUMA)?d:P_NUMA-1; }
  int je = e/nl; int ch = (N+P_NUMA-1)/P_NUMA; int d = je/ch; return (d<P_NUMA)?d:P_NUMA-1;
}
inline int numa_2d_b(int e, int B, int N, int nl) {
  int jb = e/(B*nl); int nb = N/B;
  int ch = (nb+P_NUMA-1)/P_NUMA; int d = jb/ch; return (d<P_NUMA)?d:P_NUMA-1;
}
inline int numa_2d_t(int e, int TX, int TY, int N, int nl) {
  int nx = N/TX, ny = (nl+TY-1)/TY;
  int t = e / (TY*TX);
  int yt = t % ny, xt = t / ny;
  (void)yt; int ch = (nx+P_NUMA-1)/P_NUMA; int d = xt/ch; return (d<P_NUMA)?d:P_NUMA-1;
}

inline int nu_blk_v(int V, int64_t ba, int N, int nl) {
  int e = (int)(ba*BLOCK_BYTES_G/8);
  return numa_2d(V, e, N, nl);
}
inline int nu_blk_b(int B, int64_t ba, int N, int nl) {
  int e = (int)(ba*BLOCK_BYTES_G/8);
  return numa_2d_b(e, B, N, nl);
}
inline int nu_blk_t(int TX, int TY, int64_t ba, int N, int nl) {
  int e = (int)(ba*BLOCK_BYTES_G/8);
  return numa_2d_t(e, TX, TY, N, nl);
}
inline double wn_v(int V, int64_t ba, int64_t rd, int N, int nl) {
  if (nu_blk_v(V,ba,N,nl) != NU_HOME) return GAMMA;
  return (rd < BETA) ? ALPHA : 1.0;
}
inline double wn_b(int B, int64_t ba, int64_t rd, int N, int nl) {
  if (nu_blk_b(B,ba,N,nl) != NU_HOME) return GAMMA;
  return (rd < BETA) ? ALPHA : 1.0;
}
inline double wn_t(int TX, int TY, int64_t ba, int64_t rd, int N, int nl) {
  if (nu_blk_t(TX,TY,ba,N,nl) != NU_HOME) return GAMMA;
  return (rd < BETA) ? ALPHA : 1.0;
}

static constexpr int NR = 1;
struct Ref { int a; int64_t b; };
inline void refs_v(int V, int je, int jk, int N, int nl, Ref r[NR]) {
  r[0] = {A_LVM, blk(IC_v(V, je, jk, N, nl))};
}
inline void refs_b(int B, int je, int jk, int nl, Ref r[NR]) {
  r[0] = {A_LVM, blk(IC_b(je, jk, B, nl))};
}
inline void refs_t(int TX, int TY, int je, int jk, int N, int nl, Ref r[NR]) {
  r[0] = {A_LVM, blk(IC_t(je, jk, TX, TY, N, nl))};
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

static inline void step(int W, int je0, int jk, int N, int nl,
                        BS &prev, BS &curr, StepAccum &acc,
                        int V, int B, int TX, int TY) {
  curr.clear();
  for (int w = 0; w < W; w++) {
    int je = je0 + w;
    Ref r[NR];
    if (V > 0)      refs_v(V, je, jk, N, nl, r);
    else if (B > 0) refs_b(B, je, jk, nl, r);
    else            refs_t(TX, TY, je, jk, N, nl, r);
    for (int i = 0; i < NR; i++) curr.add(r[i].a, r[i].b);
  }
  curr.fin();

  auto wnsel = [&](int64_t ba, int64_t rd) -> double {
    if (V > 0)      return wn_v(V, ba, rd, N, nl);
    else if (B > 0) return wn_b(B, ba, rd, N, nl);
    else            return wn_t(TX, TY, ba, rd, N, nl);
  };

  if (acc.T == 0) {
    int nb = curr.tot(); acc.smu += nb;
    acc.sdmin += 1.0; acc.sdmax += 1.0;
    double ws = 0;
    for (int ai = 0; ai < NUM_ARR; ai++)
      for (int64_t b : curr.a[ai]) ws += wnsel(b, INT64_MAX);
    acc.sdnuma += (nb>0) ? ws/nb : 0.0;
  } else {
    int nc = 0; double sd=0, sdx=0, sdn=0;
    for (int ai = 0; ai < NUM_ARR; ai++)
      for (int64_t b : curr.a[ai]) {
        if (prev.has(ai, b)) continue;
        nc++;
        int64_t dmin = INT64_MAX, dmax = 0;
        for (int64_t bp : prev.a[ai]) { int64_t d = std::abs(b - bp); if (d<dmin) dmin=d; if (d>dmax) dmax=d; }
        if (dmin == INT64_MAX) {
          sd += 1.0; sdx += 1.0; sdn += wnsel(b, INT64_MAX);
        } else {
          sd += (double)dmin; sdx += (double)dmax; sdn += wnsel(b, dmin) * (double)dmin;
        }
      }
    acc.smu += nc;
    if (nc > 0) { acc.sdmin += sd/nc; acc.sdmax += sdx/nc; acc.sdnuma += sdn/nc; }
  }
  acc.T++; std::swap(prev, curr);
}

static inline int jk_lo_for_nl(int nl) { int v = nl/8; return v < 3 ? 3 : v; }
static inline int jk_hi_for_nl(int nl) { return nl - 3; }

Metrics compute_slice(int V, int B, int TX, int TY, int W, int N, int nl,
                      Schedule sc, int jk_lo, int jk_hi) {
  G_SCHED = sc;
  BS prev, curr; StepAccum acc;
  for (int jk = jk_lo; jk < jk_hi; jk++) {
    /* each jk row is walked in W-wide strides; reset `prev` across jk
     * boundaries because the memory pointer jumps far away and reuse
     * is not expected across rows. */
    BS rprev, rcurr; StepAccum racc;
    for (int je = 0; je + W <= N; je += W) {
      step(W, je, jk, N, nl, rprev, rcurr, racc, V, B, TX, TY);
    }
    acc.smu += racc.smu; acc.sdmin += racc.sdmin;
    acc.sdnuma += racc.sdnuma; acc.sdmax += racc.sdmax;
    acc.T += racc.T;
  }
  (void)prev; (void)curr;
  if (!acc.T) return {};
  double d = (double)acc.T;
  return { acc.smu/d, acc.sdmin/d, acc.sdnuma/d, acc.sdmax/d,
           (acc.smu/d)*(acc.sdmin/d), (acc.smu/d)*(acc.sdnuma/d), acc.T };
}

Metrics compute_metrics(int V, int B, int TX, int TY, int W, int N, int nl, Schedule sc) {
  int jk_lo = jk_lo_for_nl(nl), jk_hi = jk_hi_for_nl(nl);
  int nj = jk_hi - jk_lo;
  if (nj <= 0) return {};
  Metrics ac = {}; int nd = 0;
  auto add = [&](Metrics &m) {
    if (!m.T) return;
    ac.mu += m.mu; ac.delta += m.delta; ac.delta_numa += m.delta_numa;
    ac.delta_max += m.delta_max; ac.mu_delta += m.mu_delta;
    ac.mu_delta_numa += m.mu_delta_numa; ac.T += m.T; nd++;
  };
  int ch = (nj + P_NUMA - 1) / P_NUMA;
  for (int d = 0; d < P_NUMA; d++) {
    int l = jk_lo + d*ch, h = std::min(jk_lo + (d+1)*ch, jk_hi);
    if (l >= h) continue;
    NU_HOME = d;
    auto m = compute_slice(V, B, TX, TY, W, N, nl, sc, l, h);
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
static const int TXV[] = {8,16,32,64};
static const int TYV[] = {8,16,32,64};
static constexpr int NTXY = 4;

struct Res {
  const char *tgt,*sch;
  int V, B, TX, TY, bb, w;
  double l1r; Metrics m;
};

int main(int argc, char **argv) {
  const char *csv = (argc > 1) ? argv[1] : nullptr;
  int N  = (argc > 2) ? atoi(argv[2]) : 81920;
  int nl = (argc > 3) ? atoi(argv[3]) : 90;
  BETA   = (argc > 4) ? atoi(argv[4]) : 1;
  ALPHA  = (argc > 5) ? atof(argv[5]) : 0.012;
  GAMMA  = (argc > 6) ? atof(argv[6]) : 1.8;
  P_NUMA = (argc > 7) ? atoi(argv[7]) : 4;
  L1_BYTES = (argc > 8) ? atoi(argv[8]) : 32768;

  Schedule sc = SCHED_OMP_FOR;
  std::vector<Res> res;

  std::string csv_def = "metrics_ln6_" + std::to_string(nl) + ".csv";
  const char *csv_path = csv ? csv : csv_def.c_str();
  FILE *fcsv = fopen(csv_path, "w");
  fprintf(fcsv, "nlev,variant,blocking,tile_x,tile_y,schedule,target,block_bytes,vector_width,"
                "mu,delta,delta_numa,delta_max,mu_delta,mu_delta_numa,l1_ratio,"
                "beta,alpha,gamma,P_NUMA,T\n");

  #pragma omp parallel
  {
    std::vector<Res> loc;
    #pragma omp for schedule(static) collapse(2)
    for (int V = 1; V <= 4; V++)
      for (int ti = 0; ti < NT; ti++) {
        int Wv = tgts[ti].w;
        if (Wv > N) continue;
        BLOCK_BYTES_G = tgts[ti].bb;
        Metrics m = compute_metrics(V, 0, 0, 0, Wv, N, nl, sc);
        loc.push_back({tgts[ti].c, sched_name[0], V, 0, 0, 0, tgts[ti].bb, Wv, 0, m});
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
        Metrics m = compute_metrics(0, B, 0, 0, Wv, N, nl, sc);
        double ws = ((double)B * 1 * 8) / (double)L1_BYTES;
        loc.push_back({tgts[ti].c, sched_name[0], 0, B, 0, 0, tgts[ti].bb, Wv, ws, m});
      }
    #pragma omp critical
    res.insert(res.end(), loc.begin(), loc.end());
  }

  #pragma omp parallel
  {
    std::vector<Res> loc;
    #pragma omp for schedule(static) collapse(2)
    for (int xi = 0; xi < NTXY; xi++)
      for (int yi = 0; yi < NTXY; yi++) {
        int TX = TXV[xi], TY = TYV[yi];
        if (N % TX != 0 || nl % TY != 0) continue;
        for (int ti = 0; ti < NT; ti++) {
          int Wv = tgts[ti].w;
          if (Wv > TX) continue;
          BLOCK_BYTES_G = tgts[ti].bb;
          Metrics m = compute_metrics(0, 0, TX, TY, Wv, N, nl, sc);
          double ws = ((double)TX * TY * 8) / (double)L1_BYTES;
          loc.push_back({tgts[ti].c, sched_name[0], 0, 0, TX, TY, tgts[ti].bb, Wv, ws, m});
        }
      }
    #pragma omp critical
    res.insert(res.end(), loc.begin(), loc.end());
  }

  for (auto &r : res) {
    if (!r.m.T) continue;
    fprintf(fcsv, "%d,%d,%d,%d,%d,%s,%s,%d,%d,"
                  "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,"
                  "%d,%.4f,%.4f,%d,%ld\n",
            nl, r.V, r.B, r.TX, r.TY, r.sch, r.tgt, r.bb, r.w,
            r.m.mu, r.m.delta, r.m.delta_numa, r.m.delta_max,
            r.m.mu_delta, r.m.mu_delta_numa, r.l1r,
            BETA, ALPHA, GAMMA, P_NUMA, (long)r.m.T);
  }
  fclose(fcsv);

  printf("\n  [ln6] N=%d nlev=%d (vert-only reduction) beta=%d alpha=%.4f gamma=%.3f P=%d\n\n",
         N, nl, BETA, ALPHA, GAMMA, P_NUMA);
  return 0;
}
