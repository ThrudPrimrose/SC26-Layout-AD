/*
 * bench_common.h -- loopnest_2 (Nest 5): direct stencil, partial vertical
 *
 *   DO jk = nflatlev_jg, nlev
 *     DO je = i_startidx, i_endidx
 *       z_w_concorr_me(je, jk) = vn(je, jk) * ddxn_z_full(je, jk)
 *                              + vt(je, jk) * ddxt_z_full(je, jk)
 *     END DO
 *   END DO
 *
 * 5 edge-dimension 2D arrays, zero indirection. All reads at same (je, jk).
 * Same layout variants (V1-V4), blocking, and (TX x TY) tiling as loopnest_1.
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

/* Partial vertical range: [NFLATLEV, nlev). For synthetic runs we pick
 * nflatlev = nlev/3 as a representative of ICON's R02B05 split (~30/90). */
static constexpr int NFLATLEV_FRAC_DEN = 3;
static inline int nflatlev_for(int nlev) { return nlev / NFLATLEV_FRAC_DEN; }

#ifdef __CUDACC__
#define HD __host__ __device__ __forceinline__
#else
#ifdef __HIP_PLATFORM_AMD__
#define HD __host__ __device__ __forceinline__
#else
#define HD inline
#endif
#endif

/* ================================================================ */
/*  2D index helpers (same family as loopnest_1)                    */
/* ================================================================ */
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

/* ================================================================ */
/*  Stencil bodies                                                   */
/* ================================================================ */
#define STENCIL_BODY(V)                                    \
  int c = IC<V>(je, jk, N_e, nlev);                        \
  out[c] = vn[c] * ddxn[c] + vt[c] * ddxt[c];

#define STENCIL_BODY_BLOCKED(B_)                           \
  int c = IC_blocked(je, jk, B_, nlev);                    \
  out[c] = vn[c] * ddxn[c] + vt[c] * ddxt[c];

#define STENCIL_BODY_TILED(TX_, TY_)                       \
  int c = IC_tiled(je, jk, TX_, TY_, N_e, nlev);           \
  out[c] = vn[c] * ddxn[c] + vt[c] * ddxt[c];

/* ================================================================ */
/*  Fill helpers                                                     */
/* ================================================================ */
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

/* ================================================================ */
/*  Layout transforms                                                */
/* ================================================================ */
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

/* ================================================================ */
/*  NUMA allocation (CPU only)                                       */
/*                                                                   */
/*  numa_alloc_unfaulted<T> / numa_dealloc<T> / first_touch_*        */
/*  come from ../../common/numa_util.h. Global policy:               */
/*  MADV_HUGEPAGE on every allocation.                               */
/* ================================================================ */
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
#endif /* !__CUDACC__ */

/* ================================================================ */
/*  BenchData                                                        */
/* ================================================================ */
struct BenchData {
  int N_e=0, nlev=0;
  size_t sz_e=0;
  int cur_V=0, cur_B=0;
  int cur_TX=0, cur_TY=0;
  double *src_vn=nullptr, *src_vt=nullptr, *src_ddxn=nullptr, *src_ddxt=nullptr;
  double *h_vn=nullptr,   *h_vt=nullptr,   *h_ddxn=nullptr,   *h_ddxt=nullptr;
  double *h_out=nullptr;
#ifndef __CUDACC__
  bool numa_owned = false;
#endif

  void alloc(int Ne, int nl) {
    N_e = Ne; nlev = nl; sz_e = (size_t)Ne*nl;
    src_vn   = new double[sz_e]; src_vt   = new double[sz_e];
    src_ddxn = new double[sz_e]; src_ddxt = new double[sz_e];
    h_vn   = new double[sz_e]; h_vt   = new double[sz_e];
    h_ddxn = new double[sz_e]; h_ddxt = new double[sz_e];
    h_out  = new double[sz_e];
  }
  void fill(int nl) {
    fill_xor(src_vn,   sz_e, 100+nl);
    fill_xor(src_vt,   sz_e, 200+nl);
    fill_xor(src_ddxn, sz_e, 300+nl);
    fill_xor(src_ddxt, sz_e, 400+nl);
  }
  void set_variant(int V, SchedKind sched = SCHED_JK_OUTER) {
    cur_V=V; cur_B=0; cur_TX=0; cur_TY=0;
    rearrange_2d(V, h_vn,   src_vn,   N_e, nlev);
    rearrange_2d(V, h_vt,   src_vt,   N_e, nlev);
    rearrange_2d(V, h_ddxn, src_ddxn, N_e, nlev);
    rearrange_2d(V, h_ddxt, src_ddxt, N_e, nlev);
#ifndef __CUDACC__
    _numa_redistribute(V, sched);
#else
    (void)sched;
#endif
  }
  void set_variant_blocked(int B, SchedKind sched = SCHED_JE_OUTER) {
    cur_V=0; cur_B=B; cur_TX=0; cur_TY=0;
    layout_2d_blocked(h_vn,   src_vn,   N_e, nlev, B);
    layout_2d_blocked(h_vt,   src_vt,   N_e, nlev, B);
    layout_2d_blocked(h_ddxn, src_ddxn, N_e, nlev, B);
    layout_2d_blocked(h_ddxt, src_ddxt, N_e, nlev, B);
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
    layout_2d_tiled(h_vn,   src_vn,   N_e, nlev, TX, TY_eff);
    layout_2d_tiled(h_vt,   src_vt,   N_e, nlev, TX, TY_eff);
    layout_2d_tiled(h_ddxn, src_ddxn, N_e, nlev, TX, TY_eff);
    layout_2d_tiled(h_ddxt, src_ddxt, N_e, nlev, TX, TY_eff);
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
    numa_dealloc(h_vn,sz_e); numa_dealloc(h_vt,sz_e);
    numa_dealloc(h_ddxn,sz_e); numa_dealloc(h_ddxt,sz_e);
    numa_dealloc(h_out,sz_e);
    numa_owned = false;
  }
  void _swap_numa(double *nv, double *nvt, double *nxn, double *nxt) {
    double *nout = numa_alloc_unfaulted<double>(sz_e);
#pragma omp parallel for schedule(static)
    for (size_t i = 0; i < sz_e; i++) nout[i] = 0.0;
    if (numa_owned) _free_numa();
    else { delete[]h_vn; delete[]h_vt; delete[]h_ddxn; delete[]h_ddxt; delete[]h_out; }
    h_vn=nv; h_vt=nvt; h_ddxn=nxn; h_ddxt=nxt; h_out=nout;
    numa_owned = true;
  }
  void _numa_redistribute(int V, SchedKind sched) {
    double *nv,*nvt,*nxn,*nxt;
    switch (V) {
      case 1: nv =redistribute_2d<1>(h_vn,N_e,nlev,sched);   nvt=redistribute_2d<1>(h_vt,N_e,nlev,sched);
              nxn=redistribute_2d<1>(h_ddxn,N_e,nlev,sched); nxt=redistribute_2d<1>(h_ddxt,N_e,nlev,sched); break;
      case 2: nv =redistribute_2d<2>(h_vn,N_e,nlev,sched);   nvt=redistribute_2d<2>(h_vt,N_e,nlev,sched);
              nxn=redistribute_2d<2>(h_ddxn,N_e,nlev,sched); nxt=redistribute_2d<2>(h_ddxt,N_e,nlev,sched); break;
      case 3: nv =redistribute_2d<3>(h_vn,N_e,nlev,sched);   nvt=redistribute_2d<3>(h_vt,N_e,nlev,sched);
              nxn=redistribute_2d<3>(h_ddxn,N_e,nlev,sched); nxt=redistribute_2d<3>(h_ddxt,N_e,nlev,sched); break;
      default:nv =redistribute_2d<4>(h_vn,N_e,nlev,sched);   nvt=redistribute_2d<4>(h_vt,N_e,nlev,sched);
              nxn=redistribute_2d<4>(h_ddxn,N_e,nlev,sched); nxt=redistribute_2d<4>(h_ddxt,N_e,nlev,sched); break;
    }
    _swap_numa(nv, nvt, nxn, nxt);
  }
  void _numa_redistribute_blocked(int B, SchedKind sched) {
    double *nv =redistribute_2d_blocked(h_vn,  N_e,nlev,B,sched);
    double *nvt=redistribute_2d_blocked(h_vt,  N_e,nlev,B,sched);
    double *nxn=redistribute_2d_blocked(h_ddxn,N_e,nlev,B,sched);
    double *nxt=redistribute_2d_blocked(h_ddxt,N_e,nlev,B,sched);
    _swap_numa(nv, nvt, nxn, nxt);
  }
  void _numa_redistribute_tiled(int TX, int TY, SchedKind sched) {
    double *nv =redistribute_2d_tiled(h_vn,  N_e,nlev,TX,TY,sched);
    double *nvt=redistribute_2d_tiled(h_vt,  N_e,nlev,TX,TY,sched);
    double *nxn=redistribute_2d_tiled(h_ddxn,N_e,nlev,TX,TY,sched);
    double *nxt=redistribute_2d_tiled(h_ddxt,N_e,nlev,TX,TY,sched);
    _swap_numa(nv, nvt, nxn, nxt);
  }
public:
#endif

  void free_all() {
#ifndef __CUDACC__
    if (numa_owned) { _free_numa(); } else {
#endif
      delete[]h_vn; delete[]h_vt; delete[]h_ddxn; delete[]h_ddxt; delete[]h_out;
#ifndef __CUDACC__
    }
#endif
    delete[]src_vn; delete[]src_vt; delete[]src_ddxn; delete[]src_ddxt;
  }
};
