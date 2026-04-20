/*
 * bench_cpu.cpp -- loopnest_3 (Nest 7) CPU benchmark.
 * Direct stencil, full vertical jk in [0, nlev):
 *   z_v_grad_w = z_v_grad_w*gradh(jk)
 *              + vn_ie*(vn_ie*invr(jk) - ft_e(je))
 *              + z_vt_ie*(z_vt_ie*invr(jk) + fn_e(je))
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

template <int V>
static void cpu_unblocked_jk_outer(
    double *__restrict w_io, const double *__restrict vn_ie, const double *__restrict z_vt_ie,
    const double *__restrict gradh, const double *__restrict invr,
    const double *__restrict ft_e,  const double *__restrict fn_e,
    int N_e, int nlev, int jk_start, int jk_end) {
#pragma omp parallel for schedule(static)
  for (int jk = jk_start; jk < jk_end; jk++)
    for (int je = 0; je < N_e; je++) { STENCIL_BODY(V) }
}
template <int V>
static void cpu_unblocked_je_outer(
    double *__restrict w_io, const double *__restrict vn_ie, const double *__restrict z_vt_ie,
    const double *__restrict gradh, const double *__restrict invr,
    const double *__restrict ft_e,  const double *__restrict fn_e,
    int N_e, int nlev, int jk_start, int jk_end) {
#pragma omp parallel for schedule(static)
  for (int je = 0; je < N_e; je++)
    for (int jk = jk_start; jk < jk_end; jk++) { STENCIL_BODY(V) }
}
template <int V>
static void cpu_unblocked_collapse2(
    double *__restrict w_io, const double *__restrict vn_ie, const double *__restrict z_vt_ie,
    const double *__restrict gradh, const double *__restrict invr,
    const double *__restrict ft_e,  const double *__restrict fn_e,
    int N_e, int nlev, int jk_start, int jk_end) {
#pragma omp parallel for collapse(2) schedule(static)
  for (int jk = jk_start; jk < jk_end; jk++)
    for (int je = 0; je < N_e; je++) { STENCIL_BODY(V) }
}

template <int B>
static void cpu_blocked_je_outer(
    double *__restrict w_io, const double *__restrict vn_ie, const double *__restrict z_vt_ie,
    const double *__restrict gradh, const double *__restrict invr,
    const double *__restrict ft_e,  const double *__restrict fn_e,
    int N_e, int nlev, int jk_start, int jk_end) {
  int nblk = N_e / B;
#pragma omp parallel for schedule(static)
  for (int jb = 0; jb < nblk; jb++)
    for (int jk = jk_start; jk < jk_end; jk++)
      for (int jl = 0; jl < B; jl++) {
        int je = jb*B + jl; STENCIL_BODY_BLOCKED(B)
      }
}
template <int B>
static void cpu_blocked_collapse2(
    double *__restrict w_io, const double *__restrict vn_ie, const double *__restrict z_vt_ie,
    const double *__restrict gradh, const double *__restrict invr,
    const double *__restrict ft_e,  const double *__restrict fn_e,
    int N_e, int nlev, int jk_start, int jk_end) {
  int nblk = N_e / B;
#pragma omp parallel for collapse(2) schedule(static)
  for (int jb = 0; jb < nblk; jb++)
    for (int jk = jk_start; jk < jk_end; jk++)
      for (int jl = 0; jl < B; jl++) {
        int je = jb*B + jl; STENCIL_BODY_BLOCKED(B)
      }
}

template <int TX, int TY>
static void cpu_tiled_je_outer(
    double *__restrict w_io, const double *__restrict vn_ie, const double *__restrict z_vt_ie,
    const double *__restrict gradh, const double *__restrict invr,
    const double *__restrict ft_e,  const double *__restrict fn_e,
    int N_e, int nlev, int jk_start, int jk_end) {
  int nx = N_e / TX;
  int ny = (nlev + TY - 1) / TY;
#pragma omp parallel for schedule(static)
  for (int xb = 0; xb < nx; xb++)
    for (int yb = 0; yb < ny; yb++) {
      int jk0 = yb*TY;
      int jk1 = jk0 + TY; if (jk1 > nlev) jk1 = nlev;
      for (int jk = jk0; jk < jk1; jk++) {
        if (jk < jk_start || jk >= jk_end) continue;
        for (int xi = 0; xi < TX; xi++) {
          int je = xb*TX + xi; STENCIL_BODY_TILED(TX, TY)
        }
      }
    }
}
template <int TX, int TY>
static void cpu_tiled_collapse2(
    double *__restrict w_io, const double *__restrict vn_ie, const double *__restrict z_vt_ie,
    const double *__restrict gradh, const double *__restrict invr,
    const double *__restrict ft_e,  const double *__restrict fn_e,
    int N_e, int nlev, int jk_start, int jk_end) {
  int nx = N_e / TX;
  int ny = (nlev + TY - 1) / TY;
#pragma omp parallel for collapse(2) schedule(static)
  for (int xb = 0; xb < nx; xb++)
    for (int yb = 0; yb < ny; yb++) {
      int jk0 = yb*TY;
      int jk1 = jk0 + TY; if (jk1 > nlev) jk1 = nlev;
      for (int jk = jk0; jk < jk1; jk++) {
        if (jk < jk_start || jk >= jk_end) continue;
        for (int xi = 0; xi < TX; xi++) {
          int je = xb*TX + xi; STENCIL_BODY_TILED(TX, TY)
        }
      }
    }
}

using KFn = void (*)(double*, const double*, const double*, const double*,
                     const double*, const double*, const double*, int, int, int, int);
static KFn kfun_unblocked[4][3] = {
  {cpu_unblocked_jk_outer<1>, cpu_unblocked_je_outer<1>, cpu_unblocked_collapse2<1>},
  {cpu_unblocked_jk_outer<2>, cpu_unblocked_je_outer<2>, cpu_unblocked_collapse2<2>},
  {cpu_unblocked_jk_outer<3>, cpu_unblocked_je_outer<3>, cpu_unblocked_collapse2<3>},
  {cpu_unblocked_jk_outer<4>, cpu_unblocked_je_outer<4>, cpu_unblocked_collapse2<4>},
};
static KFn kfun_blocked[5][2] = {
  {cpu_blocked_je_outer<8>,   cpu_blocked_collapse2<8>  },
  {cpu_blocked_je_outer<16>,  cpu_blocked_collapse2<16> },
  {cpu_blocked_je_outer<32>,  cpu_blocked_collapse2<32> },
  {cpu_blocked_je_outer<64>,  cpu_blocked_collapse2<64> },
  {cpu_blocked_je_outer<128>, cpu_blocked_collapse2<128>}
};
static KFn kfun_tiled_je[4][4] = {
  {cpu_tiled_je_outer<8,8>,  cpu_tiled_je_outer<8,16>,  cpu_tiled_je_outer<8,32>,  cpu_tiled_je_outer<8,64> },
  {cpu_tiled_je_outer<16,8>, cpu_tiled_je_outer<16,16>, cpu_tiled_je_outer<16,32>, cpu_tiled_je_outer<16,64>},
  {cpu_tiled_je_outer<32,8>, cpu_tiled_je_outer<32,16>, cpu_tiled_je_outer<32,32>, cpu_tiled_je_outer<32,64>},
  {cpu_tiled_je_outer<64,8>, cpu_tiled_je_outer<64,16>, cpu_tiled_je_outer<64,32>, cpu_tiled_je_outer<64,64>}
};
static KFn kfun_tiled_co[4][4] = {
  {cpu_tiled_collapse2<8,8>,  cpu_tiled_collapse2<8,16>,  cpu_tiled_collapse2<8,32>,  cpu_tiled_collapse2<8,64> },
  {cpu_tiled_collapse2<16,8>, cpu_tiled_collapse2<16,16>, cpu_tiled_collapse2<16,32>, cpu_tiled_collapse2<16,64>},
  {cpu_tiled_collapse2<32,8>, cpu_tiled_collapse2<32,16>, cpu_tiled_collapse2<32,32>, cpu_tiled_collapse2<32,64>},
  {cpu_tiled_collapse2<64,8>, cpu_tiled_collapse2<64,16>, cpu_tiled_collapse2<64,32>, cpu_tiled_collapse2<64,64>}
};

static inline double elapsed_ms(clk::time_point a, clk::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}

static void jacobi_flush(double *a, double *b, int n) {
#pragma omp parallel for schedule(static)
  for (int i = 0; i < n*n; i++) a[i] = 0.5 * (a[i] + b[i]);
}

static SchedKind sched_for(int V) {
  return (V <= 2) ? SCHED_JK_OUTER : SCHED_JE_OUTER;
}

static void run_variant(FILE *fcsv, int V, BenchData &bd, int nlev_end,
                        const char *dist, double *fb0, double *fb1, int FN) {
  bd.set_variant(V, sched_for(V));
  int jk0 = 0, jk1 = nlev_end;
  int ki = V - 1;
  const char *sch_names[3] = {"jk_outer", "je_outer", "collapse2"};
  for (int si = 0; si < 3; si++) {
    for (int r = 0; r < WARMUP; r++) {
      kfun_unblocked[ki][si](bd.h_w, bd.h_vie, bd.h_vti, bd.h_gradh, bd.h_invr,
                             bd.h_ft, bd.h_fn, bd.N_e, bd.nlev, jk0, jk1);
      jacobi_flush(fb0, fb1, FN);
    }
    for (int r = 0; r < NRUNS; r++) {
      auto t0 = clk::now();
      kfun_unblocked[ki][si](bd.h_w, bd.h_vie, bd.h_vti, bd.h_gradh, bd.h_invr,
                             bd.h_ft, bd.h_fn, bd.N_e, bd.nlev, jk0, jk1);
      auto t1 = clk::now();
      fprintf(fcsv, "cpu,%d,%d,%d,%s,V%d,%s,%d,%.6f\n",
              V, bd.nlev, bd.N_e, dist, V, sch_names[si], r, elapsed_ms(t0,t1));
      jacobi_flush(fb0, fb1, FN);
    }
  }
}

static void run_blocked(FILE *fcsv, int bi, BenchData &bd, int nlev_end,
                        const char *dist, double *fb0, double *fb1, int FN) {
  int B = BLOCK_SIZES[bi];
  int jk0 = 0, jk1 = nlev_end;
  const char *sch_names[2] = {"omp_for", "collapse2"};
  for (int si = 0; si < 2; si++) {
    bd.change_schedule((si == 0) ? SCHED_JE_OUTER : SCHED_COLLAPSE2);
    for (int r = 0; r < WARMUP; r++) {
      kfun_blocked[bi][si](bd.h_w, bd.h_vie, bd.h_vti, bd.h_gradh, bd.h_invr,
                           bd.h_ft, bd.h_fn, bd.N_e, bd.nlev, jk0, jk1);
      jacobi_flush(fb0, fb1, FN);
    }
    for (int r = 0; r < NRUNS; r++) {
      auto t0 = clk::now();
      kfun_blocked[bi][si](bd.h_w, bd.h_vie, bd.h_vti, bd.h_gradh, bd.h_invr,
                           bd.h_ft, bd.h_fn, bd.N_e, bd.nlev, jk0, jk1);
      auto t1 = clk::now();
      fprintf(fcsv, "cpu,0,%d,%d,%s,B%d,blocked_%s,%d,%.6f\n",
              bd.nlev, bd.N_e, dist, B, sch_names[si], r, elapsed_ms(t0,t1));
      jacobi_flush(fb0, fb1, FN);
    }
  }
}

static void run_tiled(FILE *fcsv, int txi, int tyi, BenchData &bd,
                      int nlev_end, const char *dist, double *fb0, double *fb1, int FN) {
  int TX = TILE_X_VALUES[txi];
  int TY = TILE_Y_VALUES[tyi + 1];
  int jk0 = 0, jk1 = nlev_end;
  const char *sch_names[2] = {"omp_for", "collapse2"};
  for (int si = 0; si < 2; si++) {
    KFn k = (si == 0) ? kfun_tiled_je[txi][tyi] : kfun_tiled_co[txi][tyi];
    for (int r = 0; r < WARMUP; r++) {
      k(bd.h_w, bd.h_vie, bd.h_vti, bd.h_gradh, bd.h_invr,
        bd.h_ft, bd.h_fn, bd.N_e, bd.nlev, jk0, jk1);
      jacobi_flush(fb0, fb1, FN);
    }
    for (int r = 0; r < NRUNS; r++) {
      auto t0 = clk::now();
      k(bd.h_w, bd.h_vie, bd.h_vti, bd.h_gradh, bd.h_invr,
        bd.h_ft, bd.h_fn, bd.N_e, bd.nlev, jk0, jk1);
      auto t1 = clk::now();
      fprintf(fcsv, "cpu,0,%d,%d,%s,%d,tiled_%s_ty%d,%d,%.6f\n",
              bd.nlev, bd.N_e, dist, TX, sch_names[si], TY, r, elapsed_ms(t0,t1));
      jacobi_flush(fb0, fb1, FN);
    }
  }
}

int main(int argc, char *argv[]) {
  const char *csv_path = (argc >= 2) ? argv[1] : "z_v_grad_w_full_cpu.csv";
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
    int nlev_end = nlev;

    const char *dists[3] = {"uniform", "normal_var1", "exact"};
    int ndists = have_exact ? 3 : 2;

    for (int di = 0; di < ndists; di++) {
      BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
      for (int V = 1; V <= 4; V++) run_variant(fcsv, V, bd, nlev_end, dists[di], fb0, fb1, FN);
      fflush(fcsv); bd.free_all();
    }

    for (int di = 0; di < ndists; di++) {
      for (int bi = 0; bi < N_BLOCK_SIZES; bi++) {
        int B = BLOCK_SIZES[bi];
        if (N_e % B != 0) { printf("SKIP B=%d !| N_e=%d\n", B, N_e); continue; }
        BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
        bd.set_variant_blocked(B, SCHED_JE_OUTER);
        run_blocked(fcsv, bi, bd, nlev_end, dists[di], fb0, fb1, FN);
        fflush(fcsv); bd.free_all();
      }
    }

    for (int di = 0; di < ndists; di++) {
      for (int tx_i = 0; tx_i < N_TILE_X; tx_i++) {
        int TX = TILE_X_VALUES[tx_i];
        if (N_e % TX != 0) continue;
        for (int ty_i = 0; ty_i < 4; ty_i++) {
          int TY = TILE_Y_VALUES[ty_i + 1];
          if (nlev % TY != 0) continue;
          BenchData bd; bd.alloc(N_e, nlev); bd.fill(nlev);
          bd.set_variant_tiled(TX, TY, SCHED_JE_OUTER);
          run_tiled(fcsv, tx_i, ty_i, bd, nlev_end, dists[di], fb0, fb1, FN);
          fflush(fcsv); bd.free_all();
        }
      }
    }
  }

  numa_dealloc(fb0, (size_t)FN*FN); numa_dealloc(fb1, (size_t)FN*FN);
  if (have_exact) ied.free_all();
  fclose(fcsv);
  return 0;
}
