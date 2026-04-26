/*
 * bench_common.h -- loopnest_6 (Nest 25): vertical-only level reduction.
 *
 *   DO jk = MAX(3, nrdmax_jg - 2), nlev - 3
 *     levelmask(jk) = ANY(levmask(i_startblk:i_endblk, jk))
 *   END DO
 *
 * 2 arrays:
 *   2D edge-shaped (R): levmask [N_e * nlev]
 *   1D vertical   (W): levelmask [nlev]
 *
 * Layout variants V1-V4 / 1D blocking / 2D tiling apply to `levmask`.
 * `levelmask` is 1D over jk and layout-invariant.
 *
 * The reduction axis is horizontal (je), and the vertical range is
 * partial: [max(3, nlev/8), nlev-3) stands in for the Fortran
 * MAX(3, nrdmax-2) .. nlev-3 range. The mask density is chosen
 * sparse (~15% true) so the ANY reduction is not trivially saturated.
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

static constexpr int NLEVS[] = {256};
static constexpr int N_NLEVS = 1;
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

static constexpr double MASK_P_TRUE = 0.15;

static inline int jk_lo_for(int nlev) { int v = nlev/8; return v < 3 ? 3 : v; }
static inline int jk_hi_for(int nlev) { return nlev - 3; }

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
static void fill_mask_d(double *__restrict__ arr, size_t n, unsigned seed, double p_true) {
  for (size_t i = 0; i < n; i++) {
    uint64_t h = splitmix64((uint64_t)seed * 2654435761ULL + i);
    double u = (double)(h & 0xFFFFFF) / 16777216.0;
    arr[i] = (u < p_true) ? 1.0 : 0.0;
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
    default:
#pragma omp parallel for schedule(static)
      for (int jk = 0; jk < nlev; jk++)
        for (int i = 0; i < N; i++) { int x=IC<V>(i,jk,N,nlev); dst[x]=src[x]; }
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
  /* 2D edge-shaped (read-only reduction source) */
  double *src_lvm=nullptr;
  double *h_lvm=nullptr;
  /* 1D vertical output */
  double *h_lvout=nullptr;
#ifndef __CUDACC__
  bool numa_owned = false;
#endif

  void alloc(int Ne, int nl) {
    N_e = Ne; nlev = nl; sz_e = (size_t)Ne*nl;
    src_lvm = new double[sz_e];
    h_lvm   = new double[sz_e];
    h_lvout = new double[nl];
  }
  void fill(int nl) {
    fill_mask_d(src_lvm, sz_e, 100+nl, MASK_P_TRUE);
    for (int jk = 0; jk < nl; jk++) h_lvout[jk] = 0.0;
  }
  void set_variant(int V, SchedKind sched = SCHED_JK_OUTER) {
    cur_V=V; cur_B=0; cur_TX=0; cur_TY=0;
    rearrange_2d(V, h_lvm, src_lvm, N_e, nlev);
#ifndef __CUDACC__
    _numa_redistribute(V, sched);
#else
    (void)sched;
#endif
  }
  void set_variant_blocked(int B, SchedKind sched = SCHED_JE_OUTER) {
    cur_V=0; cur_B=B; cur_TX=0; cur_TY=0;
    layout_2d_blocked(h_lvm, src_lvm, N_e, nlev, B);
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
    layout_2d_tiled(h_lvm, src_lvm, N_e, nlev, TX, TY_eff);
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
    numa_dealloc(h_lvm, sz_e);
    numa_dealloc(h_lvout, nlev);
    numa_owned = false;
  }
  void _swap_numa(double *nlvm) {
    double *nlvout = redistribute_1d(h_lvout, nlev);
    if (numa_owned) _free_numa();
    else {
      delete[]h_lvm; delete[]h_lvout;
    }
    h_lvm = nlvm; h_lvout = nlvout;
    numa_owned = true;
  }
  void _numa_redistribute(int V, SchedKind sched) {
    double *m;
    switch (V) {
      case 1: m=redistribute_2d<1>(h_lvm,N_e,nlev,sched); break;
      case 2: m=redistribute_2d<2>(h_lvm,N_e,nlev,sched); break;
      case 3: m=redistribute_2d<3>(h_lvm,N_e,nlev,sched); break;
      default:m=redistribute_2d<4>(h_lvm,N_e,nlev,sched); break;
    }
    _swap_numa(m);
  }
  void _numa_redistribute_blocked(int B, SchedKind sched) {
    double *m = redistribute_2d_blocked(h_lvm, N_e, nlev, B, sched);
    _swap_numa(m);
  }
  void _numa_redistribute_tiled(int TX, int TY, SchedKind sched) {
    double *m = redistribute_2d_tiled(h_lvm, N_e, nlev, TX, TY, sched);
    _swap_numa(m);
  }
public:
#endif

  void free_all() {
#ifndef __CUDACC__
    if (numa_owned) { _free_numa(); } else {
#endif
      delete[]h_lvm; delete[]h_lvout;
#ifndef __CUDACC__
    }
#endif
    delete[]src_lvm;
  }
};

/* Reduction body: read levmask(je, jk) and OR-accumulate into a scalar.
 * The driver wraps these in a jk-outer loop over [jk_lo, jk_hi). */
#define STENCIL_V(V) \
  { int idx = IC<V>(je, jk, N_e, nlev); \
    acc = acc || (lvm[idx] != 0.0); }

#define STENCIL_B(B) \
  { int idx = IC_blocked(je, jk, B, nlev); \
    acc = acc || (lvm[idx] != 0.0); }

#define STENCIL_T(TX, TY) \
  { int idx = IC_tiled(je, jk, TX, TY, N_e, nlev); \
    acc = acc || (lvm[idx] != 0.0); }
