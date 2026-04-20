/*
 * bench_common.h -- loopnest_4 (Nest 24): indirect stencil + CFL clip,
 *                   partial vertical.
 *
 *   DO jk = jk_lo, jk_hi
 *     IF (lvlmask(jk) .or. lvlmask(jk+1)) THEN
 *       DO je = 1, N_e
 *         w_con_e = c_lin_e(je,1)*z_w(icidx(je,1), jk) + c_lin_e(je,2)*z_w(icidx(je,2), jk)
 *         IF (|w_con_e| > cfl_w_limit * ddqz(je,jk)) THEN
 *           difcoef = sfex * min(0.85 - cfl*dt, |w_con_e|*dt/ddqz(je,jk) - cfl*dt)
 *           sum = geofac(je,1)*vn(je,jk)
 *               + sum_{k=1..4} geofac(je,k+1)*vn(iqidx(je,k), jk)
 *               + tang(je)*invp(je)*(zeta(ividx(je,2), jk) - zeta(ividx(je,1), jk))
 *           ddt(je,jk) += difcoef * area(je) * sum
 *         END IF
 *       END DO
 *     END IF
 *   END DO
 *
 * Synthetic simplification: z_w and zeta share the edge-shaped 2D layout;
 * neighbour indices are flat edge-ids in [0, N_e). All 2D arrays get
 * V1-V4 / blocked / tiled transforms; 1D tables are layout-invariant.
 */
#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>

#ifndef __CUDACC__
#include <sys/mman.h>
#include <omp.h>
#endif

static constexpr int NLEVS[]        = {96};
static constexpr int N_NLEVS        = 1;
static constexpr int WARMUP         = 5;
static constexpr int NRUNS          = 100;
static constexpr int BLOCK_SIZES[]  = {8, 16, 32, 64, 128};
static constexpr int N_BLOCK_SIZES  = 5;
static constexpr int NUMA_DOMAINS   = 4;

static constexpr int TILE_X_VALUES[]   = {8, 16, 32, 64};
static constexpr int N_TILE_X          = 4;
static constexpr int TILE_Y_VALUES[]   = {0, 8, 16, 32, 64};
static constexpr int N_TILE_Y          = 5;
static constexpr int TILE_Y_MATCH_NLEV = 0;

static constexpr double CFL_W_LIMIT = 0.65;
static constexpr double DTIME       = 60.0;
static constexpr double SFEX        = 0.05;

/* Partial vertical range: [jk_lo, jk_hi) with jk_lo ~ nlev/8, jk_hi ~ 7*nlev/8,
 * representing MAX(3, nrdmax-2) .. nlev-4. */
static inline int jk_lo_for(int nlev) { int v = nlev/8;    return v < 1 ? 1 : v; }
static inline int jk_hi_for(int nlev) { int v = (7*nlev)/8; return v > nlev-1 ? nlev-1 : v; }

#ifdef __CUDACC__
#define HD __host__ __device__ __forceinline__
#else
#ifdef __HIP_PLATFORM_AMD__
#define HD __host__ __device__ __forceinline__
#else
#define HD inline
#endif
#endif

template <int V> HD int IC(int i, int jk, int N, int nlev) {
  if constexpr (V <= 2) return i + jk * N;
  else                  return jk + i * nlev;
}
HD int IC_blocked(int i, int jk, int B, int nlev) {
  return (i % B) + jk * B + (i / B) * B * nlev;
}
HD int IC_tiled(int i, int jk, int TX, int TY, int N, int nlev) {
  int n_y = (nlev + TY - 1) / TY;
  int xt = i / TX, yt = jk / TY;
  int xi = i - xt * TX, yi = jk - yt * TY;
  return xt * (n_y * TY * TX) + yt * (TY * TX) + yi * TX + xi;
}
HD int tile_y_effective(int TY, int nlev) {
  return (TY == TILE_Y_MATCH_NLEV) ? nlev : TY;
}

static inline uint64_t splitmix64(uint64_t x) {
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}
static void fill_xor(double *arr, size_t n, unsigned seed) {
  for (size_t i = 0; i < n; i++) {
    uint64_t h = splitmix64((uint64_t)seed * 2654435761ULL + i);
    arr[i] = (double)(int64_t)(h & 0xFFFFF) / 100000.0 - 5.0;
  }
}
static void fill_idx(int *arr, size_t n, int lo, int hi, unsigned seed) {
  for (size_t i = 0; i < n; i++) {
    uint64_t h = splitmix64((uint64_t)seed * 2654435761ULL + i);
    int range = hi - lo;
    arr[i] = lo + (int)(h % (uint64_t)range);
  }
}
static void fill_mask(int *arr, size_t n, unsigned seed, double p_true = 0.6) {
  for (size_t i = 0; i < n; i++) {
    uint64_t h = splitmix64((uint64_t)seed * 2654435761ULL + i);
    double u = (double)(h & 0xFFFFFF) / 16777216.0;
    arr[i] = (u < p_true) ? 1 : 0;
  }
}

enum SchedKind { SCHED_JK_OUTER=0, SCHED_JE_OUTER=1, SCHED_COLLAPSE2=2,
                 SCHED_NUMA4=3, SCHED_COLLAPSE2_JE=4 };

template <int V> static void layout_2d(double *dst, const double *src,
                                       int N, int nlev) {
#pragma omp parallel for schedule(static) collapse(2)
  for (int jk = 0; jk < nlev; jk++)
    for (int i = 0; i < N; i++)
      dst[IC<V>(i,jk,N,nlev)] = src[i + jk*N];
}
static void rearrange_2d(int V, double *dst, const double *src, int N, int nlev) {
  switch (V) {
    case 1: layout_2d<1>(dst,src,N,nlev); break;
    case 2: layout_2d<2>(dst,src,N,nlev); break;
    case 3: layout_2d<3>(dst,src,N,nlev); break;
    case 4: layout_2d<4>(dst,src,N,nlev); break;
  }
}
static void layout_2d_blocked(double *dst, const double *src,
                              int N, int nlev, int B) {
#pragma omp parallel for schedule(static) collapse(2)
  for (int jk = 0; jk < nlev; jk++)
    for (int i = 0; i < N; i++)
      dst[IC_blocked(i,jk,B,nlev)] = src[i + jk*N];
}
static void layout_2d_tiled(double *dst, const double *src,
                            int N, int nlev, int TX, int TY) {
#pragma omp parallel for schedule(static) collapse(2)
  for (int jk = 0; jk < nlev; jk++)
    for (int i = 0; i < N; i++)
      dst[IC_tiled(i,jk,TX,TY,N,nlev)] = src[i + jk*N];
}

#ifndef __CUDACC__
template <typename T> static T *numa_alloc_unfaulted(size_t count) {
  size_t bytes = count * sizeof(T);
  void *p = mmap(nullptr, bytes, PROT_READ|PROT_WRITE,
                 MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
  if (p == MAP_FAILED) { perror("mmap"); std::abort(); }
  madvise(p, bytes, MADV_HUGEPAGE);
  return (T *)p;
}
template <typename T> static void numa_dealloc(T *p, size_t count) {
  if (p) munmap(p, count * sizeof(T));
}
template <int V>
static double *redistribute_2d(const double *src, int N, int nlev, SchedKind sched) {
  double *dst = numa_alloc_unfaulted<double>((size_t)N*nlev);
  switch (sched) {
    case SCHED_COLLAPSE2:
#pragma omp parallel for schedule(static) collapse(2)
      for (int jk = 0; jk < nlev; jk++)
        for (int i = 0; i < N; i++) { int x=IC<V>(i,jk,N,nlev); dst[x]=src[x]; }
      break;
    case SCHED_JE_OUTER:
#pragma omp parallel for schedule(static)
      for (int i = 0; i < N; i++)
        for (int jk = 0; jk < nlev; jk++) { int x=IC<V>(i,jk,N,nlev); dst[x]=src[x]; }
      break;
    default:
#pragma omp parallel for schedule(static)
      for (int jk = 0; jk < nlev; jk++)
        for (int i = 0; i < N; i++) { int x=IC<V>(i,jk,N,nlev); dst[x]=src[x]; }
      break;
  }
  return dst;
}
static double *redistribute_2d_blocked(const double *src, int N, int nlev,
                                       int B, SchedKind) {
  double *dst = numa_alloc_unfaulted<double>((size_t)N*nlev);
  int nblk = N / B;
#pragma omp parallel for schedule(static)
  for (int jb = 0; jb < nblk; jb++)
    for (int jk = 0; jk < nlev; jk++)
      for (int jl = 0; jl < B; jl++) { int x=IC_blocked(jb*B+jl,jk,B,nlev); dst[x]=src[x]; }
  return dst;
}
static double *redistribute_2d_tiled(const double *src, int N, int nlev,
                                     int TX, int TY, SchedKind) {
  double *dst = numa_alloc_unfaulted<double>((size_t)N*nlev);
  int nx = N / TX, ny = (nlev + TY - 1) / TY;
#pragma omp parallel for schedule(static)
  for (int xb = 0; xb < nx; xb++)
    for (int yb = 0; yb < ny; yb++) {
      int jk0 = yb*TY, jk1 = std::min(jk0+TY, nlev);
      for (int jk = jk0; jk < jk1; jk++)
        for (int xi = 0; xi < TX; xi++) {
          int x = IC_tiled(xb*TX + xi, jk, TX, TY, N, nlev); dst[x] = src[x];
        }
    }
  return dst;
}
template <typename T>
static T *redistribute_1d(const T *src, size_t N) {
  T *dst = numa_alloc_unfaulted<T>(N);
#pragma omp parallel for schedule(static)
  for (size_t i = 0; i < N; i++) dst[i] = src[i];
  return dst;
}
#endif /* !__CUDACC__ */

struct BenchData {
  int N_e=0, nlev=0;
  size_t sz_e=0;
  int cur_V=0, cur_B=0, cur_TX=0, cur_TY=0;
  /* 2D edge-shaped arrays: ddt (RMW), vn, z_w, zeta, ddqz */
  double *src_ddt=nullptr, *src_vn=nullptr, *src_zw=nullptr, *src_zeta=nullptr, *src_ddqz=nullptr;
  double *h_ddt=nullptr,   *h_vn=nullptr,   *h_zw=nullptr,   *h_zeta=nullptr,   *h_ddqz=nullptr;
  /* horizontal-only: c_lin_e[N_e*2], geofac[N_e*5], area[N_e], tang[N_e], invp[N_e] */
  double *h_cle=nullptr, *h_gf=nullptr;
  double *h_area=nullptr, *h_tang=nullptr, *h_invp=nullptr;
  /* indirection tables (flat edge indices) */
  int *h_ici=nullptr;   /* [N_e * 2] */
  int *h_iqi=nullptr;   /* [N_e * 4] */
  int *h_ivi=nullptr;   /* [N_e * 2] */
  /* vertical mask */
  int *h_lvlm=nullptr;  /* [nlev+1] */
#ifndef __CUDACC__
  bool numa_owned = false;
#endif

  void alloc(int Ne, int nl) {
    N_e = Ne; nlev = nl; sz_e = (size_t)Ne*nl;
    src_ddt = new double[sz_e]; src_vn = new double[sz_e];
    src_zw  = new double[sz_e]; src_zeta = new double[sz_e]; src_ddqz = new double[sz_e];
    h_ddt = new double[sz_e]; h_vn = new double[sz_e];
    h_zw  = new double[sz_e]; h_zeta = new double[sz_e]; h_ddqz = new double[sz_e];
    h_cle  = new double[(size_t)Ne*2];
    h_gf   = new double[(size_t)Ne*5];
    h_area = new double[Ne]; h_tang = new double[Ne]; h_invp = new double[Ne];
    h_ici = new int[(size_t)Ne*2];
    h_iqi = new int[(size_t)Ne*4];
    h_ivi = new int[(size_t)Ne*2];
    h_lvlm = new int[nl+1];
  }
  void fill(int nl) {
    fill_xor(src_ddt,  sz_e, 100+nl);
    fill_xor(src_vn,   sz_e, 200+nl);
    fill_xor(src_zw,   sz_e, 300+nl);
    fill_xor(src_zeta, sz_e, 400+nl);
    fill_xor(src_ddqz, sz_e, 500+nl);
    for (size_t i = 0; i < (size_t)N_e*5; i++) h_gf[i]  = 0.1 * (double)((i * 2654435761ULL) & 0xFFFF) / 65536.0;
    for (size_t i = 0; i < (size_t)N_e*2; i++) h_cle[i] = 0.5;
    fill_xor(h_area, N_e, 800+nl);
    fill_xor(h_tang, N_e, 900+nl);
    fill_xor(h_invp, N_e, 1000+nl);
    fill_idx(h_ici, (size_t)N_e*2, 0, N_e, 1100+nl);
    fill_idx(h_iqi, (size_t)N_e*4, 0, N_e, 1200+nl);
    fill_idx(h_ivi, (size_t)N_e*2, 0, N_e, 1300+nl);
    /* ensure ddqz is strictly positive to avoid divide-by-zero in kernel */
    for (size_t i = 0; i < sz_e; i++) src_ddqz[i] = std::fabs(src_ddqz[i]) + 1e-2;
    fill_mask(h_lvlm, nl+1, 1400+nl, 0.6);
  }
  void set_variant(int V, SchedKind sched = SCHED_JK_OUTER) {
    cur_V=V; cur_B=0; cur_TX=0; cur_TY=0;
    rearrange_2d(V, h_ddt,  src_ddt,  N_e, nlev);
    rearrange_2d(V, h_vn,   src_vn,   N_e, nlev);
    rearrange_2d(V, h_zw,   src_zw,   N_e, nlev);
    rearrange_2d(V, h_zeta, src_zeta, N_e, nlev);
    rearrange_2d(V, h_ddqz, src_ddqz, N_e, nlev);
#ifndef __CUDACC__
    _numa_redistribute(V, sched);
#else
    (void)sched;
#endif
  }
  void set_variant_blocked(int B, SchedKind sched = SCHED_JE_OUTER) {
    cur_V=0; cur_B=B; cur_TX=0; cur_TY=0;
    layout_2d_blocked(h_ddt,  src_ddt,  N_e, nlev, B);
    layout_2d_blocked(h_vn,   src_vn,   N_e, nlev, B);
    layout_2d_blocked(h_zw,   src_zw,   N_e, nlev, B);
    layout_2d_blocked(h_zeta, src_zeta, N_e, nlev, B);
    layout_2d_blocked(h_ddqz, src_ddqz, N_e, nlev, B);
#ifndef __CUDACC__
    _numa_redistribute_blocked(B, sched);
#else
    (void)sched;
#endif
  }
  void set_variant_tiled(int TX, int TY, SchedKind sched = SCHED_JE_OUTER) {
    int TY_eff = tile_y_effective(TY, nlev);
    if (TY == TILE_Y_MATCH_NLEV || TY_eff == nlev) {
      set_variant_blocked(TX, sched); cur_TX = TX; cur_TY = nlev; return;
    }
    cur_V=0; cur_B=0; cur_TX=TX; cur_TY=TY_eff;
    layout_2d_tiled(h_ddt,  src_ddt,  N_e, nlev, TX, TY_eff);
    layout_2d_tiled(h_vn,   src_vn,   N_e, nlev, TX, TY_eff);
    layout_2d_tiled(h_zw,   src_zw,   N_e, nlev, TX, TY_eff);
    layout_2d_tiled(h_zeta, src_zeta, N_e, nlev, TX, TY_eff);
    layout_2d_tiled(h_ddqz, src_ddqz, N_e, nlev, TX, TY_eff);
#ifndef __CUDACC__
    _numa_redistribute_tiled(TX, TY_eff, sched);
#else
    (void)sched;
#endif
  }

#ifndef __CUDACC__
  void change_schedule(SchedKind sched) {
    if (cur_TX > 0 && cur_TY > 0 && cur_TY != nlev) _numa_redistribute_tiled(cur_TX, cur_TY, sched);
    else if (cur_B > 0)                             _numa_redistribute_blocked(cur_B, sched);
    else                                            _numa_redistribute(cur_V, sched);
  }
private:
  void _free_numa() {
    if (!numa_owned) return;
    numa_dealloc(h_ddt,sz_e); numa_dealloc(h_vn,sz_e); numa_dealloc(h_zw,sz_e);
    numa_dealloc(h_zeta,sz_e); numa_dealloc(h_ddqz,sz_e);
    numa_dealloc(h_cle,(size_t)N_e*2); numa_dealloc(h_gf,(size_t)N_e*5);
    numa_dealloc(h_area,N_e); numa_dealloc(h_tang,N_e); numa_dealloc(h_invp,N_e);
    numa_owned = false;
  }
  void _swap_numa(double *nddt, double *nvn, double *nzw, double *nzeta, double *nddqz) {
    double *ncle  = redistribute_1d(h_cle,  (size_t)N_e*2);
    double *ngf   = redistribute_1d(h_gf,   (size_t)N_e*5);
    double *narea = redistribute_1d(h_area, N_e);
    double *ntang = redistribute_1d(h_tang, N_e);
    double *ninvp = redistribute_1d(h_invp, N_e);
    if (numa_owned) _free_numa();
    else {
      delete[]h_ddt; delete[]h_vn; delete[]h_zw; delete[]h_zeta; delete[]h_ddqz;
      delete[]h_cle; delete[]h_gf; delete[]h_area; delete[]h_tang; delete[]h_invp;
    }
    h_ddt = nddt; h_vn = nvn; h_zw = nzw; h_zeta = nzeta; h_ddqz = nddqz;
    h_cle = ncle; h_gf = ngf; h_area = narea; h_tang = ntang; h_invp = ninvp;
    numa_owned = true;
  }
  void _numa_redistribute(int V, SchedKind sched) {
    double *d,*v,*z,*ze,*dq;
    switch (V) {
      case 1: d=redistribute_2d<1>(h_ddt,N_e,nlev,sched); v=redistribute_2d<1>(h_vn,N_e,nlev,sched);
              z=redistribute_2d<1>(h_zw,N_e,nlev,sched); ze=redistribute_2d<1>(h_zeta,N_e,nlev,sched);
              dq=redistribute_2d<1>(h_ddqz,N_e,nlev,sched); break;
      case 2: d=redistribute_2d<2>(h_ddt,N_e,nlev,sched); v=redistribute_2d<2>(h_vn,N_e,nlev,sched);
              z=redistribute_2d<2>(h_zw,N_e,nlev,sched); ze=redistribute_2d<2>(h_zeta,N_e,nlev,sched);
              dq=redistribute_2d<2>(h_ddqz,N_e,nlev,sched); break;
      case 3: d=redistribute_2d<3>(h_ddt,N_e,nlev,sched); v=redistribute_2d<3>(h_vn,N_e,nlev,sched);
              z=redistribute_2d<3>(h_zw,N_e,nlev,sched); ze=redistribute_2d<3>(h_zeta,N_e,nlev,sched);
              dq=redistribute_2d<3>(h_ddqz,N_e,nlev,sched); break;
      default:d=redistribute_2d<4>(h_ddt,N_e,nlev,sched); v=redistribute_2d<4>(h_vn,N_e,nlev,sched);
              z=redistribute_2d<4>(h_zw,N_e,nlev,sched); ze=redistribute_2d<4>(h_zeta,N_e,nlev,sched);
              dq=redistribute_2d<4>(h_ddqz,N_e,nlev,sched); break;
    }
    _swap_numa(d,v,z,ze,dq);
  }
  void _numa_redistribute_blocked(int B, SchedKind sched) {
    double *d =redistribute_2d_blocked(h_ddt,  N_e,nlev,B,sched);
    double *v =redistribute_2d_blocked(h_vn,   N_e,nlev,B,sched);
    double *z =redistribute_2d_blocked(h_zw,   N_e,nlev,B,sched);
    double *ze=redistribute_2d_blocked(h_zeta, N_e,nlev,B,sched);
    double *dq=redistribute_2d_blocked(h_ddqz, N_e,nlev,B,sched);
    _swap_numa(d,v,z,ze,dq);
  }
  void _numa_redistribute_tiled(int TX, int TY, SchedKind sched) {
    double *d =redistribute_2d_tiled(h_ddt,  N_e,nlev,TX,TY,sched);
    double *v =redistribute_2d_tiled(h_vn,   N_e,nlev,TX,TY,sched);
    double *z =redistribute_2d_tiled(h_zw,   N_e,nlev,TX,TY,sched);
    double *ze=redistribute_2d_tiled(h_zeta, N_e,nlev,TX,TY,sched);
    double *dq=redistribute_2d_tiled(h_ddqz, N_e,nlev,TX,TY,sched);
    _swap_numa(d,v,z,ze,dq);
  }
public:
#endif

  void free_all() {
#ifndef __CUDACC__
    if (numa_owned) { _free_numa(); } else {
#endif
      delete[]h_ddt; delete[]h_vn; delete[]h_zw; delete[]h_zeta; delete[]h_ddqz;
      delete[]h_cle; delete[]h_gf; delete[]h_area; delete[]h_tang; delete[]h_invp;
#ifndef __CUDACC__
    }
#endif
    delete[]src_ddt; delete[]src_vn; delete[]src_zw; delete[]src_zeta; delete[]src_ddqz;
    delete[]h_ici; delete[]h_iqi; delete[]h_ivi; delete[]h_lvlm;
  }
};
