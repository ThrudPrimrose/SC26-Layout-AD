/*
 * bench_cpu.cpp -- loopnest_5 (Nest 2) CPU benchmark.
 * Horizontal-only boundary kernel writing to jk=0 and jk=nlev (nlevp1-1).
 */
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <omp.h>
#include <vector>

#include "bench_common.h"
#include "../loopnest_1/icon_data_loader.h"

using clk = std::chrono::high_resolution_clock;

#define KARGS \
  double *__restrict vnie, double *__restrict zvt, double *__restrict zk, \
  const double *__restrict vn, const double *__restrict vt, \
  const double *__restrict ubc, const double *__restrict wgt, \
  int N_e, int nlev, int nlevp1

template <int V> static void cpu_unblocked(KARGS) {
#pragma omp parallel for schedule(static)
  for (int je = 0; je < N_e; je++) { STENCIL_V(V) }
}
template <int B> static void cpu_blocked(KARGS) {
  int nblk = N_e / B;
#pragma omp parallel for schedule(static)
  for (int jb = 0; jb < nblk; jb++)
    for (int jl = 0; jl < B; jl++) { int je = jb*B + jl; STENCIL_B(B) }
}

using KFn = void (*)(double*, double*, double*, const double*, const double*,
                     const double*, const double*, int, int, int);
static KFn kfun_unblocked[4] = {
  cpu_unblocked<1>, cpu_unblocked<2>, cpu_unblocked<3>, cpu_unblocked<4>
};
static KFn kfun_blocked[5] = {
  cpu_blocked<8>, cpu_blocked<16>, cpu_blocked<32>, cpu_blocked<64>, cpu_blocked<128>
};

static inline double elapsed_ms(clk::time_point a, clk::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}
static void jacobi_flush(double *a, double *b, int n) {
#pragma omp parallel for schedule(static)
  for (int i = 0; i < n*n; i++) a[i] = 0.5 * (a[i] + b[i]);
}

#define CALL_K(fn) fn(bd.h_vnie, bd.h_zvt, bd.h_zk, bd.h_vn, bd.h_vt, \
                      bd.h_ubc, bd.h_wgt, bd.N_e, bd.nlev, bd.nlevp1)

static void run_variant(FILE *fcsv, int V, BenchData &bd, const char *dist,
                        double *fb0, double *fb1, int FN) {
  bd.set_variant(V, (V <= 2) ? SCHED_JE_OUTER : SCHED_JE_OUTER);
  int ki = V - 1;
  for (int r = 0; r < WARMUP; r++) {
    CALL_K(kfun_unblocked[ki]); jacobi_flush(fb0, fb1, FN);
  }
  for (int r = 0; r < NRUNS; r++) {
    auto t0 = clk::now();
    CALL_K(kfun_unblocked[ki]);
    auto t1 = clk::now();
    fprintf(fcsv, "cpu,%d,%d,%d,%s,V%d,je_outer,%d,%.6f\n",
            V, bd.nlev, bd.N_e, dist, V, r, elapsed_ms(t0,t1));
    jacobi_flush(fb0, fb1, FN);
  }
}
static void run_blocked(FILE *fcsv, int bi, BenchData &bd, const char *dist,
                        double *fb0, double *fb1, int FN) {
  int B = BLOCK_SIZES[bi];
  for (int r = 0; r < WARMUP; r++) {
    CALL_K(kfun_blocked[bi]); jacobi_flush(fb0, fb1, FN);
  }
  for (int r = 0; r < NRUNS; r++) {
    auto t0 = clk::now();
    CALL_K(kfun_blocked[bi]);
    auto t1 = clk::now();
    fprintf(fcsv, "cpu,0,%d,%d,%s,B%d,blocked_omp_for,%d,%.6f\n",
            bd.nlev, bd.N_e, dist, B, r, elapsed_ms(t0,t1));
    jacobi_flush(fb0, fb1, FN);
  }
}

int main(int argc, char *argv[]) {
  const char *csv_path = (argc >= 2) ? argv[1] : "vn_ie_boundary_cpu.csv";
  FILE *fcsv = fopen(csv_path, "w"); if (!fcsv) { perror("fopen"); return 1; }
  fprintf(fcsv, "backend,V,nlev,N_e,cell_dist,layout,schedule,run_id,time_ms\n");

  int step = (argc >= 3) ? atoi(argv[2]) : 9;
  std::string gp = icon_global_path(step), pp = icon_patch_path(step);
  int icon_nproma = icon_read_nproma(gp.c_str());
  IconEdgeData ied;
  bool have_exact = (icon_nproma > 0) && icon_load_patch(pp.c_str(), icon_nproma, ied);

  int N_e = have_exact ? ied.n_edges : 122880;
  const int FN = 16384;
  double *fb0 = numa_alloc_unfaulted<double>((size_t)FN*FN);
  double *fb1 = numa_alloc_unfaulted<double>((size_t)FN*FN);
#pragma omp parallel for schedule(static)
  for (size_t i = 0; i < (size_t)FN*FN; i++) { fb0[i]=0.0; fb1[i]=1.0; }

  for (int ni = 0; ni < N_NLEVS; ni++) {
    int nlev = NLEVS[ni];

    const char *dists[3] = {"uniform", "normal_var1", "exact"};
    int ndists = have_exact ? 3 : 2;

    for (int di = 0; di < ndists; di++) {
      BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
      for (int V = 1; V <= 4; V++) run_variant(fcsv, V, bd, dists[di], fb0, fb1, FN);
      fflush(fcsv); bd.free_all();
    }

    for (int di = 0; di < ndists; di++) {
      for (int bi = 0; bi < N_BLOCK_SIZES; bi++) {
        int B = BLOCK_SIZES[bi];
        if (N_e % B != 0) { printf("SKIP B=%d !| N_e=%d\n", B, N_e); continue; }
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant_blocked(B, SCHED_JE_OUTER);
        run_blocked(fcsv, bi, bd, dists[di], fb0, fb1, FN);
        fflush(fcsv); bd.free_all();
      }
    }
  }

  numa_dealloc(fb0, (size_t)FN*FN); numa_dealloc(fb1, (size_t)FN*FN);
  if (have_exact) ied.free_all();
  fclose(fcsv);
  return 0;
}
