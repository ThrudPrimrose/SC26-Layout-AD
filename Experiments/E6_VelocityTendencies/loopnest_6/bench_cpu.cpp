/*
 * bench_cpu.cpp -- loopnest_6 (Nest 25) CPU benchmark.
 * Vertical-only level reduction: for each jk in [jk_lo, jk_hi),
 *   levelmask(jk) = OR over all je of levmask(je, jk)
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

#define KARGS \
  double *__restrict lvout, const double *__restrict lvm, \
  int N_e, int nlev, int jk_start, int jk_end

template <int V> static void cpu_unblocked_jk_outer(KARGS) {
#pragma omp parallel for schedule(static)
  for (int jk = jk_start; jk < jk_end; jk++) {
    bool acc = false;
    for (int je = 0; je < N_e; je++) { STENCIL_V(V) }
    lvout[jk] = acc ? 1.0 : 0.0;
  }
}
template <int V> static void cpu_unblocked_collapse2(KARGS) {
  /* Parallel over (jk, jb-chunk), merging with atomic OR into lvout[jk]. */
  const int CHUNK = 1024;
  int nc = (N_e + CHUNK - 1) / CHUNK;
#pragma omp parallel for schedule(static)
  for (int jk = jk_start; jk < jk_end; jk++) lvout[jk] = 0.0;
#pragma omp parallel for collapse(2) schedule(static)
  for (int jk = jk_start; jk < jk_end; jk++) {
    for (int c = 0; c < nc; c++) {
      int j0 = c*CHUNK, j1 = std::min(j0+CHUNK, N_e);
      bool acc = false;
      for (int je = j0; je < j1; je++) { STENCIL_V(V) }
      if (acc) {
#pragma omp atomic write
        lvout[jk] = 1.0;
      }
    }
  }
}

template <int B> static void cpu_blocked_jk_outer(KARGS) {
  int nblk = N_e / B;
#pragma omp parallel for schedule(static)
  for (int jk = jk_start; jk < jk_end; jk++) {
    bool acc = false;
    for (int jb = 0; jb < nblk; jb++)
      for (int jl = 0; jl < B; jl++) { int je = jb*B + jl; STENCIL_B(B) }
    lvout[jk] = acc ? 1.0 : 0.0;
  }
}
template <int B> static void cpu_blocked_collapse2(KARGS) {
  int nblk = N_e / B;
#pragma omp parallel for schedule(static)
  for (int jk = jk_start; jk < jk_end; jk++) lvout[jk] = 0.0;
#pragma omp parallel for collapse(2) schedule(static)
  for (int jk = jk_start; jk < jk_end; jk++) {
    for (int jb = 0; jb < nblk; jb++) {
      bool acc = false;
      for (int jl = 0; jl < B; jl++) { int je = jb*B + jl; STENCIL_B(B) }
      if (acc) {
#pragma omp atomic write
        lvout[jk] = 1.0;
      }
    }
  }
}

template <int TX, int TY> static void cpu_tiled_jk_outer(KARGS) {
  int nx = N_e / TX;
  int ny = (nlev + TY - 1) / TY;
#pragma omp parallel for schedule(static)
  for (int jk = jk_start; jk < jk_end; jk++) lvout[jk] = 0.0;
#pragma omp parallel for collapse(2) schedule(static)
  for (int xb = 0; xb < nx; xb++)
    for (int yb = 0; yb < ny; yb++) {
      int jk0 = yb*TY, jk1 = std::min(jk0 + TY, nlev);
      for (int jk = jk0; jk < jk1; jk++) {
        if (jk < jk_start || jk >= jk_end) continue;
        bool acc = false;
        for (int xi = 0; xi < TX; xi++) { int je = xb*TX + xi; STENCIL_T(TX, TY) }
        if (acc) {
#pragma omp atomic write
          lvout[jk] = 1.0;
        }
      }
    }
}

using KFn = void (*)(double*, const double*, int, int, int, int);

static KFn kfun_unblocked[4][2] = {
  {cpu_unblocked_jk_outer<1>, cpu_unblocked_collapse2<1>},
  {cpu_unblocked_jk_outer<2>, cpu_unblocked_collapse2<2>},
  {cpu_unblocked_jk_outer<3>, cpu_unblocked_collapse2<3>},
  {cpu_unblocked_jk_outer<4>, cpu_unblocked_collapse2<4>}
};
static KFn kfun_blocked[3][2] = {
  {cpu_blocked_jk_outer<8>,   cpu_blocked_collapse2<8>  },
  {cpu_blocked_jk_outer<16>,  cpu_blocked_collapse2<16> },
  {cpu_blocked_jk_outer<32>,  cpu_blocked_collapse2<32> },
};
static KFn kfun_tiled[3][3] = {
  {cpu_tiled_jk_outer<8,8>,  cpu_tiled_jk_outer<8,16>,  cpu_tiled_jk_outer<8,32> },
  {cpu_tiled_jk_outer<16,8>, cpu_tiled_jk_outer<16,16>, cpu_tiled_jk_outer<16,32>},
  {cpu_tiled_jk_outer<32,8>, cpu_tiled_jk_outer<32,16>, cpu_tiled_jk_outer<32,32>},
};

static inline double elapsed_ms(clk::time_point a, clk::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}
/* Canonical Jacobi-2D cache flush (see ../../common/jacobi_flush.h). */
static void jacobi_flush(double *, double *, int) { flush_jacobi(); }
static SchedKind sched_for(int V) { return (V <= 2) ? SCHED_JK_OUTER : SCHED_JE_OUTER; }

#define CALL_K(fn) fn(bd.h_lvout, bd.h_lvm, bd.N_e, bd.nlev, jk0, jk1)

static void run_variant(FILE *fcsv, int V, BenchData &bd,
                        int jk0, int jk1, const char *dist,
                        double *fb0, double *fb1, int FN) {
  bd.set_variant(V, sched_for(V));
  int ki = V - 1;
  const char *sch_names[2] = {"jk_outer", "collapse2"};
  for (int si = 0; si < 2; si++) {
    for (int r = 0; r < WARMUP; r++) {
      CALL_K(kfun_unblocked[ki][si]); jacobi_flush(fb0, fb1, FN);
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
  const char *sch_names[2] = {"jk_outer", "collapse2"};
  for (int si = 0; si < 2; si++) {
    for (int r = 0; r < WARMUP; r++) {
      CALL_K(kfun_blocked[bi][si]); jacobi_flush(fb0, fb1, FN);
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
  KFn k = kfun_tiled[txi][tyi];
  for (int r = 0; r < WARMUP; r++) {
    CALL_K(k); jacobi_flush(fb0, fb1, FN);
  }
  for (int r = 0; r < NRUNS; r++) {
    auto t0 = clk::now();
    CALL_K(k);
    auto t1 = clk::now();
    fprintf(fcsv, "cpu,0,%d,%d,%s,%d,tiled_ty%d,%d,%.6f\n",
            bd.nlev, bd.N_e, dist, TX, TY, r, elapsed_ms(t0,t1));
    jacobi_flush(fb0, fb1, FN);
  }
}

int main(int argc, char *argv[]) {
  const char *csv_path = (argc >= 2) ? argv[1] : "levelmask_cpu.csv";
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
      for (int V = 1; V <= 4; V++) run_variant(fcsv, V, bd, jk0, jk1, dists[di], fb0, fb1, FN);
      fflush(fcsv); bd.free_all();
    }

    for (int di = 0; di < ndists; di++) {
      for (int bi = 0; bi < N_BLOCK_SIZES; bi++) {
        int B = BLOCK_SIZES[bi];
        if (N_e % B != 0) { printf("SKIP B=%d !| N_e=%d\n", B, N_e); continue; }
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant_blocked(B, SCHED_JE_OUTER);
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
