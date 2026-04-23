/*
 * bench_common.h -- loopnest_5 (Nest 2): horizontal-only boundary kernel.
 *
 *   DO je = 1, N_e
 *     vn_ie(je, 1)      = vn_ie_ubc(je, 1) + dt_lin*vn_ie_ubc(je, 2)
 *     z_vt_ie(je, 1)    = vt(je, 1)
 *     z_kin_hor_e(je,1) = 0.5*(vn(je,1)**2 + vt(je,1)**2)
 *     vn_ie(je, nlevp1) = wgtfacq(je,1)*vn(je, nlev)
 *                       + wgtfacq(je,2)*vn(je, nlev-1)
 *                       + wgtfacq(je,3)*vn(je, nlev-2)
 *   END DO
 *
 * 7 arrays:
 *   2D edge-shaped (5): vn_ie (W top+bot), z_vt_ie (W), z_kin_hor_e (W),
 *                       vn (R), vt (R)
 *   horizontal-only (2): vn_ie_ubc (N_e*2), wgtfacq_e (N_e*3)
 *
 * 2D arrays sized at N_e * (nlev+1) so level index nlev is addressable.
 *
 * No inner vertical loop: layout variants V1-V4 and 1D blocking are
 * swept; TY tiling is not meaningful (single-pass horizontal) and is
 * omitted from the sweep.
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

static constexpr double DT_LINIT_UBC = 0.125;

static inline int nlevp1_for(int nlev) { return nlev + 1; }

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
template <typename T>
static T *redistribute_1d(const T *src, size_t N) {
  T *dst = numa_alloc_unfaulted<T>(N);
#pragma omp parallel for schedule(static)
  for (size_t i = 0; i < N; i++) dst[i] = src[i];
  return dst;
}
#endif /* !__CUDACC__ */

struct BenchData {
  int N_e=0, nlev=0, nlevp1=0;
  size_t sz_e=0;
  int cur_V=0, cur_B=0;
  /* 2D edge-shaped arrays (N_e * nlevp1): vn_ie, z_vt_ie, z_kin, vn, vt */
  double *src_vnie=nullptr, *src_zvt=nullptr, *src_zk=nullptr, *src_vn=nullptr, *src_vt=nullptr;
  double *h_vnie=nullptr,   *h_zvt=nullptr,   *h_zk=nullptr,   *h_vn=nullptr,   *h_vt=nullptr;
  /* horizontal-only */
  double *h_ubc=nullptr; /* [N_e * 2] */
  double *h_wgt=nullptr; /* [N_e * 3] */
#ifndef __CUDACC__
  bool numa_owned = false;
#endif

  void alloc(int Ne, int nl) {
    N_e = Ne; nlev = nl; nlevp1 = nl + 1;
    sz_e = (size_t)Ne * nlevp1;
    src_vnie = new double[sz_e]; src_zvt = new double[sz_e];
    src_zk   = new double[sz_e]; src_vn  = new double[sz_e]; src_vt = new double[sz_e];
    h_vnie = new double[sz_e]; h_zvt = new double[sz_e];
    h_zk   = new double[sz_e]; h_vn  = new double[sz_e]; h_vt = new double[sz_e];
    h_ubc = new double[(size_t)Ne*2];
    h_wgt = new double[(size_t)Ne*3];
  }
  void fill(int nl) {
    fill_xor(src_vnie, sz_e, 100+nl);
    fill_xor(src_zvt,  sz_e, 200+nl);
    fill_xor(src_zk,   sz_e, 300+nl);
    fill_xor(src_vn,   sz_e, 400+nl);
    fill_xor(src_vt,   sz_e, 500+nl);
    fill_xor(h_ubc, (size_t)N_e*2, 600+nl);
    fill_xor(h_wgt, (size_t)N_e*3, 700+nl);
  }
  void set_variant(int V, SchedKind sched = SCHED_JK_OUTER) {
    cur_V=V; cur_B=0;
    rearrange_2d(V, h_vnie, src_vnie, N_e, nlevp1);
    rearrange_2d(V, h_zvt,  src_zvt,  N_e, nlevp1);
    rearrange_2d(V, h_zk,   src_zk,   N_e, nlevp1);
    rearrange_2d(V, h_vn,   src_vn,   N_e, nlevp1);
    rearrange_2d(V, h_vt,   src_vt,   N_e, nlevp1);
#ifndef __CUDACC__
    _numa_redistribute(V, sched);
#else
    (void)sched;
#endif
  }
  void set_variant_blocked(int B, SchedKind sched = SCHED_JE_OUTER) {
    cur_V=0; cur_B=B;
    layout_2d_blocked(h_vnie, src_vnie, N_e, nlevp1, B);
    layout_2d_blocked(h_zvt,  src_zvt,  N_e, nlevp1, B);
    layout_2d_blocked(h_zk,   src_zk,   N_e, nlevp1, B);
    layout_2d_blocked(h_vn,   src_vn,   N_e, nlevp1, B);
    layout_2d_blocked(h_vt,   src_vt,   N_e, nlevp1, B);
#ifndef __CUDACC__
    _numa_redistribute_blocked(B, sched);
#else
    (void)sched;
#endif
  }

#ifndef __CUDACC__
  void change_schedule(SchedKind sched) {
    if (cur_B > 0) _numa_redistribute_blocked(cur_B, sched);
    else           _numa_redistribute(cur_V, sched);
  }
private:
  void _free_numa() {
    if (!numa_owned) return;
    numa_dealloc(h_vnie, sz_e); numa_dealloc(h_zvt, sz_e);
    numa_dealloc(h_zk,   sz_e); numa_dealloc(h_vn,  sz_e); numa_dealloc(h_vt, sz_e);
    numa_dealloc(h_ubc, (size_t)N_e*2);
    numa_dealloc(h_wgt, (size_t)N_e*3);
    numa_owned = false;
  }
  void _swap_numa(double *a, double *b, double *c, double *d, double *e) {
    double *u = redistribute_1d(h_ubc, (size_t)N_e*2);
    double *w = redistribute_1d(h_wgt, (size_t)N_e*3);
    if (numa_owned) _free_numa();
    else {
      delete[]h_vnie; delete[]h_zvt; delete[]h_zk; delete[]h_vn; delete[]h_vt;
      delete[]h_ubc; delete[]h_wgt;
    }
    h_vnie=a; h_zvt=b; h_zk=c; h_vn=d; h_vt=e;
    h_ubc=u; h_wgt=w;
    numa_owned = true;
  }
  void _numa_redistribute(int V, SchedKind sched) {
    double *a,*b,*c,*d,*e;
    switch (V) {
      case 1: a=redistribute_2d<1>(h_vnie,N_e,nlevp1,sched); b=redistribute_2d<1>(h_zvt,N_e,nlevp1,sched);
              c=redistribute_2d<1>(h_zk,N_e,nlevp1,sched);   d=redistribute_2d<1>(h_vn,N_e,nlevp1,sched);
              e=redistribute_2d<1>(h_vt,N_e,nlevp1,sched); break;
      case 2: a=redistribute_2d<2>(h_vnie,N_e,nlevp1,sched); b=redistribute_2d<2>(h_zvt,N_e,nlevp1,sched);
              c=redistribute_2d<2>(h_zk,N_e,nlevp1,sched);   d=redistribute_2d<2>(h_vn,N_e,nlevp1,sched);
              e=redistribute_2d<2>(h_vt,N_e,nlevp1,sched); break;
      case 3: a=redistribute_2d<3>(h_vnie,N_e,nlevp1,sched); b=redistribute_2d<3>(h_zvt,N_e,nlevp1,sched);
              c=redistribute_2d<3>(h_zk,N_e,nlevp1,sched);   d=redistribute_2d<3>(h_vn,N_e,nlevp1,sched);
              e=redistribute_2d<3>(h_vt,N_e,nlevp1,sched); break;
      default:a=redistribute_2d<4>(h_vnie,N_e,nlevp1,sched); b=redistribute_2d<4>(h_zvt,N_e,nlevp1,sched);
              c=redistribute_2d<4>(h_zk,N_e,nlevp1,sched);   d=redistribute_2d<4>(h_vn,N_e,nlevp1,sched);
              e=redistribute_2d<4>(h_vt,N_e,nlevp1,sched); break;
    }
    _swap_numa(a,b,c,d,e);
  }
  void _numa_redistribute_blocked(int B, SchedKind sched) {
    double *a =redistribute_2d_blocked(h_vnie, N_e,nlevp1,B,sched);
    double *b =redistribute_2d_blocked(h_zvt,  N_e,nlevp1,B,sched);
    double *c =redistribute_2d_blocked(h_zk,   N_e,nlevp1,B,sched);
    double *d =redistribute_2d_blocked(h_vn,   N_e,nlevp1,B,sched);
    double *e =redistribute_2d_blocked(h_vt,   N_e,nlevp1,B,sched);
    _swap_numa(a,b,c,d,e);
  }
public:
#endif

  void free_all() {
#ifndef __CUDACC__
    if (numa_owned) { _free_numa(); } else {
#endif
      delete[]h_vnie; delete[]h_zvt; delete[]h_zk; delete[]h_vn; delete[]h_vt;
      delete[]h_ubc;  delete[]h_wgt;
#ifndef __CUDACC__
    }
#endif
    delete[]src_vnie; delete[]src_zvt; delete[]src_zk; delete[]src_vn; delete[]src_vt;
  }
};

#define STENCIL_V(V) \
  { int ct = IC<V>(je, 0,        N_e, nlevp1); \
    int cb = IC<V>(je, nlevp1-1, N_e, nlevp1); \
    int cm1 = IC<V>(je, nlevp1-2, N_e, nlevp1); \
    int cm2 = IC<V>(je, nlevp1-3, N_e, nlevp1); \
    int cm3 = IC<V>(je, nlevp1-4, N_e, nlevp1); \
    double u0 = ubc[je*2+0], u1 = ubc[je*2+1]; \
    vnie[ct] = u0 + DT_LINIT_UBC * u1; \
    double vtv = vt[ct]; \
    zvt[ct]   = vtv; \
    double vnv = vn[ct]; \
    zk[ct]    = 0.5 * (vnv*vnv + vtv*vtv); \
    vnie[cb]  = wgt[je*3+0]*vn[cm1] + wgt[je*3+1]*vn[cm2] + wgt[je*3+2]*vn[cm3]; }

#define STENCIL_B(B) \
  { int ct  = IC_blocked(je, 0,        B, nlevp1); \
    int cb  = IC_blocked(je, nlevp1-1, B, nlevp1); \
    int cm1 = IC_blocked(je, nlevp1-2, B, nlevp1); \
    int cm2 = IC_blocked(je, nlevp1-3, B, nlevp1); \
    int cm3 = IC_blocked(je, nlevp1-4, B, nlevp1); \
    double u0 = ubc[je*2+0], u1 = ubc[je*2+1]; \
    vnie[ct] = u0 + DT_LINIT_UBC * u1; \
    double vtv = vt[ct]; \
    zvt[ct]   = vtv; \
    double vnv = vn[ct]; \
    zk[ct]    = 0.5 * (vnv*vnv + vtv*vtv); \
    vnie[cb]  = wgt[je*3+0]*vn[cm1] + wgt[je*3+1]*vn[cm2] + wgt[je*3+2]*vn[cm3]; }
