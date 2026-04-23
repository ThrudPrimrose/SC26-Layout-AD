/*
 * bench_common.h -- loopnest_3 (Nest 7): direct stencil, full vertical
 *
 *   DO jk = 1, nlev
 *     DO je = i_startidx, i_endidx
 *       z_v_grad_w(je,jk) = z_v_grad_w(je,jk)*gradh(jk)
 *                         + vn_ie(je,jk)*(vn_ie(je,jk)*invr(jk) - ft_e(je))
 *                         + z_vt_ie(je,jk)*(z_vt_ie(je,jk)*invr(jk) + fn_e(je))
 *     END DO
 *   END DO
 *
 * 3 same-shape edge 2D arrays (z_v_grad_w RMW, vn_ie, z_vt_ie)
 * + 2 1D-vertical arrays (gradh, invr, length nlev)
 * + 2 1D-horizontal arrays (ft_e, fn_e, length N_e).
 * Layout variants V1-V4, blocking, and (TX x TY) tiling apply only to
 * the 2D edge arrays; 1D arrays are layout-invariant.
 */
#pragma once

/* Pull the GPU runtime in BEFORE the HD macro so __forceinline__, blockIdx etc. are declared. */
#if defined(__CUDACC__)
#  include <cuda_runtime.h>
#elif defined(__HIP_PLATFORM_AMD__) || defined(__HIP__)
#  include <hip/hip_runtime.h>
#endif


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

static constexpr int NLEVS[] = {90, 96, 128, 256};
static constexpr int N_NLEVS = 4;
static constexpr int WARMUP         = 5;
static constexpr int NRUNS          = 100;
static constexpr int BLOCK_SIZES[] = {8, 16, 32, 48, 64, 96, 128};
static constexpr int N_BLOCK_SIZES = 7;
static constexpr int NUMA_DOMAINS   = 4;

static constexpr int TILE_X_VALUES[]   = {8, 16, 32, 64, 128};
static constexpr int N_TILE_X          = 5;
static constexpr int TILE_Y_VALUES[]   = {0, 8, 16, 32, 64, 128};
static constexpr int N_TILE_Y          = 6;
static constexpr int TILE_Y_MATCH_NLEV = 0;

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

#define STENCIL_BODY(V)                                                    \
  int c = IC<V>(je, jk, N_e, nlev);                                        \
  double w = w_io[c], vie = vn_ie[c], vti = z_vt_ie[c];                    \
  double iv = invr[jk];                                                    \
  w_io[c] = w*gradh[jk]                                                    \
          + vie*(vie*iv - ft_e[je])                                        \
          + vti*(vti*iv + fn_e[je]);

#define STENCIL_BODY_BLOCKED(B_)                                           \
  int c = IC_blocked(je, jk, B_, nlev);                                    \
  double w = w_io[c], vie = vn_ie[c], vti = z_vt_ie[c];                    \
  double iv = invr[jk];                                                    \
  w_io[c] = w*gradh[jk]                                                    \
          + vie*(vie*iv - ft_e[je])                                        \
          + vti*(vti*iv + fn_e[je]);

#define STENCIL_BODY_TILED(TX_, TY_)                                       \
  int c = IC_tiled(je, jk, TX_, TY_, N_e, nlev);                           \
  double w = w_io[c], vie = vn_ie[c], vti = z_vt_ie[c];                    \
  double iv = invr[jk];                                                    \
  w_io[c] = w*gradh[jk]                                                    \
          + vie*(vie*iv - ft_e[je])                                        \
          + vti*(vti*iv + fn_e[je]);

static inline uint64_t splitmix64(uint64_t x) {
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}
static void fill_xor(double *__restrict__ arr, size_t n, unsigned seed) {
  for (size_t i = 0; i < n; i++) {
    uint64_t h = splitmix64((uint64_t)seed * 2654435761ULL + i);
    arr[i] = (double)(int64_t)(h & 0xFFFFF) / 100000.0 - 5.0;
  }
}

enum SchedKind { SCHED_JK_OUTER=0, SCHED_JE_OUTER=1, SCHED_COLLAPSE2=2,
                 SCHED_NUMA4=3, SCHED_COLLAPSE2_JE=4 };

template <int V> static void layout_2d(double *__restrict__ dst, const double *__restrict__ src,
                                       int N, int nlev) {
#pragma omp parallel for schedule(static) collapse(2)
  for (int jk = 0; jk < nlev; jk++)
    for (int i = 0; i < N; i++)
      dst[IC<V>(i,jk,N,nlev)] = src[i + jk*N];
}
static void rearrange_2d(int V, double *__restrict__ dst, const double *__restrict__ src, int N, int nlev) {
  switch (V) {
    case 1: layout_2d<1>(dst,src,N,nlev); break;
    case 2: layout_2d<2>(dst,src,N,nlev); break;
    case 3: layout_2d<3>(dst,src,N,nlev); break;
    case 4: layout_2d<4>(dst,src,N,nlev); break;
  }
}
static void layout_2d_blocked(double *__restrict__ dst, const double *__restrict__ src,
                              int N, int nlev, int B) {
#pragma omp parallel for schedule(static) collapse(2)
  for (int jk = 0; jk < nlev; jk++)
    for (int i = 0; i < N; i++)
      dst[IC_blocked(i,jk,B,nlev)] = src[i + jk*N];
}
static void layout_2d_tiled(double *__restrict__ dst, const double *__restrict__ src,
                            int N, int nlev, int TX, int TY) {
#pragma omp parallel for schedule(static) collapse(2)
  for (int jk = 0; jk < nlev; jk++)
    for (int i = 0; i < N; i++)
      dst[IC_tiled(i,jk,TX,TY,N,nlev)] = src[i + jk*N];
}

/* numa_alloc_unfaulted<T> / numa_dealloc<T> / first_touch_* come from
 * ../../common/numa_util.h. Global policy: MADV_HUGEPAGE on every alloc. */
#ifndef __CUDACC__
#include "../../common/numa_util.h"

template <int V>
static double *redistribute_2d(const double *__restrict__ src, int N, int nlev, SchedKind sched) {
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
    case SCHED_JK_OUTER:
#pragma omp parallel for schedule(static)
      for (int jk = 0; jk < nlev; jk++)
        for (int i = 0; i < N; i++) { int x=IC<V>(i,jk,N,nlev); dst[x]=src[x]; }
      break;
    case SCHED_NUMA4: {
      constexpr int ND = NUMA_DOMAINS;
      int tpd = std::max(1, omp_get_max_threads() / ND);
      int chunk = N / ND;
      #pragma omp parallel
      {
        int tid = omp_get_thread_num();
        int d = std::min(tid / tpd, ND - 1);
        int i0 = d * chunk;
        int i1 = (d == ND-1) ? N : i0 + chunk;
        int ltid = tid - d * tpd;
        int ln = (d == ND-1) ? (omp_get_num_threads() - d*tpd) : tpd;
        int my0 = i0 + (int)((long long)(i1-i0) * ltid / ln);
        int my1 = i0 + (int)((long long)(i1-i0) * (ltid+1) / ln);
        for (int i = my0; i < my1; i++)
          for (int jk = 0; jk < nlev; jk++) { int x=IC<V>(i,jk,N,nlev); dst[x]=src[x]; }
      }
      break;
    }
    case SCHED_COLLAPSE2_JE:
#pragma omp parallel for schedule(static) collapse(2)
      for (int i = 0; i < N; i++)
        for (int jk = 0; jk < nlev; jk++) { int x=IC<V>(i,jk,N,nlev); dst[x]=src[x]; }
      break;
  }
  return dst;
}
static double *redistribute_2d_blocked(const double *__restrict__ src, int N, int nlev,
                                       int B, SchedKind) {
  double *dst = numa_alloc_unfaulted<double>((size_t)N*nlev);
  int nblk = N / B;
#pragma omp parallel for schedule(static)
  for (int jb = 0; jb < nblk; jb++)
    for (int jk = 0; jk < nlev; jk++)
      for (int jl = 0; jl < B; jl++) { int x=IC_blocked(jb*B+jl,jk,B,nlev); dst[x]=src[x]; }
  return dst;
}
static double *redistribute_2d_tiled(const double *__restrict__ src, int N, int nlev,
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
static T *redistribute_1d(const T *src, int N) {
  T *dst = numa_alloc_unfaulted<T>(N);
#pragma omp parallel for schedule(static)
  for (int i = 0; i < N; i++) dst[i] = src[i];
  return dst;
}
#endif /* !__CUDACC__ */

struct BenchData {
  int N_e=0, nlev=0;
  size_t sz_e=0;
  int cur_V=0, cur_B=0, cur_TX=0, cur_TY=0;
  /* sources (always row-major, plain new[]) */
  double *src_w=nullptr, *src_vie=nullptr, *src_vti=nullptr;
  double *src_gradh=nullptr, *src_invr=nullptr;
  double *src_ft=nullptr,    *src_fn=nullptr;
  /* layout-applied + (CPU) NUMA-redistributed copies */
  double *h_w=nullptr,   *h_vie=nullptr, *h_vti=nullptr;
  double *h_gradh=nullptr, *h_invr=nullptr;
  double *h_ft=nullptr,    *h_fn=nullptr;
#ifndef __CUDACC__
  bool numa_owned = false;
#endif

  void alloc(int Ne, int nl) {
    N_e = Ne; nlev = nl; sz_e = (size_t)Ne*nl;
    src_w   = new double[sz_e]; src_vie = new double[sz_e]; src_vti = new double[sz_e];
    src_gradh = new double[nl]; src_invr = new double[nl];
    src_ft    = new double[Ne]; src_fn   = new double[Ne];
    h_w   = new double[sz_e]; h_vie = new double[sz_e]; h_vti = new double[sz_e];
    h_gradh = new double[nl]; h_invr = new double[nl];
    h_ft    = new double[Ne]; h_fn   = new double[Ne];
  }
  void fill(int nl) {
    fill_xor(src_w,   sz_e, 100+nl);
    fill_xor(src_vie, sz_e, 200+nl);
    fill_xor(src_vti, sz_e, 300+nl);
    fill_xor(src_gradh, nl,    400+nl);
    fill_xor(src_invr,  nl,    500+nl);
    fill_xor(src_ft,    N_e,   600+nl);
    fill_xor(src_fn,    N_e,   700+nl);
  }
  void _copy_1d() {
    std::memcpy(h_gradh, src_gradh, nlev*sizeof(double));
    std::memcpy(h_invr,  src_invr,  nlev*sizeof(double));
    std::memcpy(h_ft,    src_ft,    N_e*sizeof(double));
    std::memcpy(h_fn,    src_fn,    N_e*sizeof(double));
  }
  void set_variant(int V, SchedKind sched = SCHED_JK_OUTER) {
    cur_V=V; cur_B=0; cur_TX=0; cur_TY=0;
    rearrange_2d(V, h_w,   src_w,   N_e, nlev);
    rearrange_2d(V, h_vie, src_vie, N_e, nlev);
    rearrange_2d(V, h_vti, src_vti, N_e, nlev);
    _copy_1d();
#ifndef __CUDACC__
    _numa_redistribute(V, sched);
#else
    (void)sched;
#endif
  }
  void set_variant_blocked(int B, SchedKind sched = SCHED_JE_OUTER) {
    cur_V=0; cur_B=B; cur_TX=0; cur_TY=0;
    layout_2d_blocked(h_w,   src_w,   N_e, nlev, B);
    layout_2d_blocked(h_vie, src_vie, N_e, nlev, B);
    layout_2d_blocked(h_vti, src_vti, N_e, nlev, B);
    _copy_1d();
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
    layout_2d_tiled(h_w,   src_w,   N_e, nlev, TX, TY_eff);
    layout_2d_tiled(h_vie, src_vie, N_e, nlev, TX, TY_eff);
    layout_2d_tiled(h_vti, src_vti, N_e, nlev, TX, TY_eff);
    _copy_1d();
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
    numa_dealloc(h_w,sz_e); numa_dealloc(h_vie,sz_e); numa_dealloc(h_vti,sz_e);
    numa_dealloc(h_gradh,nlev); numa_dealloc(h_invr,nlev);
    numa_dealloc(h_ft,N_e); numa_dealloc(h_fn,N_e);
    numa_owned = false;
  }
  void _swap_numa(double *nw, double *nvie, double *nvti) {
    double *ngh = redistribute_1d(h_gradh, nlev);
    double *niv = redistribute_1d(h_invr,  nlev);
    double *nft = redistribute_1d(h_ft,    N_e);
    double *nfn = redistribute_1d(h_fn,    N_e);
    if (numa_owned) _free_numa();
    else {
      delete[]h_w; delete[]h_vie; delete[]h_vti;
      delete[]h_gradh; delete[]h_invr;
      delete[]h_ft; delete[]h_fn;
    }
    h_w = nw; h_vie = nvie; h_vti = nvti;
    h_gradh = ngh; h_invr = niv;
    h_ft = nft; h_fn = nfn;
    numa_owned = true;
  }
  void _numa_redistribute(int V, SchedKind sched) {
    double *nw, *nvie, *nvti;
    switch (V) {
      case 1: nw =redistribute_2d<1>(h_w,N_e,nlev,sched);   nvie=redistribute_2d<1>(h_vie,N_e,nlev,sched); nvti=redistribute_2d<1>(h_vti,N_e,nlev,sched); break;
      case 2: nw =redistribute_2d<2>(h_w,N_e,nlev,sched);   nvie=redistribute_2d<2>(h_vie,N_e,nlev,sched); nvti=redistribute_2d<2>(h_vti,N_e,nlev,sched); break;
      case 3: nw =redistribute_2d<3>(h_w,N_e,nlev,sched);   nvie=redistribute_2d<3>(h_vie,N_e,nlev,sched); nvti=redistribute_2d<3>(h_vti,N_e,nlev,sched); break;
      default:nw =redistribute_2d<4>(h_w,N_e,nlev,sched);   nvie=redistribute_2d<4>(h_vie,N_e,nlev,sched); nvti=redistribute_2d<4>(h_vti,N_e,nlev,sched); break;
    }
    _swap_numa(nw, nvie, nvti);
  }
  void _numa_redistribute_blocked(int B, SchedKind sched) {
    double *nw  =redistribute_2d_blocked(h_w,  N_e,nlev,B,sched);
    double *nvie=redistribute_2d_blocked(h_vie,N_e,nlev,B,sched);
    double *nvti=redistribute_2d_blocked(h_vti,N_e,nlev,B,sched);
    _swap_numa(nw, nvie, nvti);
  }
  void _numa_redistribute_tiled(int TX, int TY, SchedKind sched) {
    double *nw  =redistribute_2d_tiled(h_w,  N_e,nlev,TX,TY,sched);
    double *nvie=redistribute_2d_tiled(h_vie,N_e,nlev,TX,TY,sched);
    double *nvti=redistribute_2d_tiled(h_vti,N_e,nlev,TX,TY,sched);
    _swap_numa(nw, nvie, nvti);
  }
public:
#endif

  void free_all() {
#ifndef __CUDACC__
    if (numa_owned) { _free_numa(); } else {
#endif
      delete[]h_w; delete[]h_vie; delete[]h_vti;
      delete[]h_gradh; delete[]h_invr;
      delete[]h_ft; delete[]h_fn;
#ifndef __CUDACC__
    }
#endif
    delete[]src_w; delete[]src_vie; delete[]src_vti;
    delete[]src_gradh; delete[]src_invr;
    delete[]src_ft; delete[]src_fn;
  }
};
