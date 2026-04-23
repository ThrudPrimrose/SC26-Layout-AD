/*
 * bench_stream.cpp -- NUMA-aware CPU STREAM sweep (scale-add RMW) on an
 * N x N fp32 matrix. Kernel:  A[y,x] = s * A[y,x] + B[y,x]
 * Traffic: 2 loads + 1 store per fp32 = 12 bytes per element.
 *
 * Modelled after E1_MatrixAdd/bench_cpu.cpp: uses the shared
 * ../common/numa_util.h allocators, the canonical cache-flush kernel
 * from ../common/jacobi_flush.h, NREP=100/NWARMUP=5, and writes one
 * CSV row per repetition for violin plots.
 *
 * NUMA model:
 *   The matrix is first-touched as a 2x2 grid of quadrants (one per
 *   NUMA domain on beverin: 4 domains x 24 Zen4 cores = 96 threads ->
 *   24 threads per quadrant). OMP_PLACES in common/setup_beverin.sh
 *   pins each group to NUMA-local cores.
 *
 * Variants benchmarked:
 *   - 1d_rows_static       : OMP for over outer rows, schedule(static)
 *   - 1d_collapse_static   : OMP for collapse(2) over (y,x), static
 *   - 1d_flat_static       : flat 1D partition among threads
 *   - 1d_rows_dynamic      : rows, schedule(dynamic, large chunk)
 *   - 2x2_numa_tiled       : each thread stays inside its NUMA quadrant,
 *                            sweep inner tile (TILE_Y, TILE_X)
 *
 * Every variant is run twice -- once with a canonical 8192^2 Jacobi
 * cache flush between reps (flush=yes) and once without (flush=no).
 * The "no flush" pass matches the old common/bench_stream.cpp
 * methodology and reports the real achievable peak on a warm cache;
 * the "yes flush" pass matches the cold-cache methodology used by
 * every E1-E6 bench. Headline statistic is the arithmetic *mean*
 * across the NREP reps (not the median), because for NUMA we care
 * about the achievable peak rather than a robust central estimate.
 *
 * Output CSV (one row per repetition):
 *   variant,TILE_Y,TILE_X,N,rep,threads,time_s,bw_gbs,checksum,status,flush
 *
 * Usage: bench_stream <csv_file> [N1d=16384] [nthreads=0=env]
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>

#include <omp.h>

#include "../common/jacobi_flush.h"  /* also transitively pulls numa_util.h */

#define NREP    100
#define NWARMUP 5

using Clock = std::chrono::high_resolution_clock;
static inline double elapsed_sec(Clock::time_point t0, Clock::time_point t1) {
    return std::chrono::duration<double>(t1 - t0).count();
}

/* ── 2x2 NUMA first-touch ─────────────────────────────────────────────
 * P threads split into 4 groups of P/4 (one per NUMA quadrant). Each
 * group row-partitions its quadrant among its threads so every page
 * is first-touched by a NUMA-local thread.
 */
static void first_touch_2x2(float *__restrict__ a, size_t N1d, float val) {
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int P   = omp_get_num_threads();
        int Pq  = P / 4;
        int q   = tid / Pq;
        int lid = tid % Pq;
        int qy  = q / 2;
        int qx  = q % 2;

        size_t H   = N1d / 2;
        size_t y0  = (size_t)qy * H;
        size_t x0  = (size_t)qx * H;
        size_t y1  = y0 + H;
        size_t x1  = x0 + H;

        size_t rows_per = H / Pq;
        size_t ly0 = y0 + (size_t)lid * rows_per;
        size_t ly1 = (lid == Pq - 1) ? y1 : ly0 + rows_per;

        for (size_t y = ly0; y < ly1; y++)
            for (size_t x = x0; x < x1; x++)
                a[y * N1d + x] = val;
    }
}

/* ── kernels ──────────────────────────────────────────────────────── */

static void k_1d_rows_static(float *__restrict__ A, const float *__restrict__ B,
                             size_t N1d) {
    const float s = 1.0001f;
    #pragma omp parallel for schedule(static)
    for (size_t y = 0; y < N1d; y++) {
        size_t row = y * N1d;
        #pragma omp simd
        for (size_t x = 0; x < N1d; x++) {
            size_t i = row + x;
            A[i] = s * A[i] + B[i];
        }
    }
}

static void k_1d_collapse_static(float *__restrict__ A, const float *__restrict__ B,
                                 size_t N1d) {
    const float s = 1.0001f;
    #pragma omp parallel for collapse(2) schedule(static)
    for (size_t y = 0; y < N1d; y++)
        for (size_t x = 0; x < N1d; x++) {
            size_t i = y * N1d + x;
            A[i] = s * A[i] + B[i];
        }
}

static void k_1d_flat_static(float *__restrict__ A, const float *__restrict__ B,
                             size_t N1d) {
    const float s = 1.0001f;
    size_t N = N1d * N1d;
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int P   = omp_get_num_threads();
        size_t lo = (size_t)N * tid / P;
        size_t hi = (size_t)N * (tid + 1) / P;
        #pragma omp simd
        for (size_t i = lo; i < hi; i++) A[i] = s * A[i] + B[i];
    }
}

static void k_1d_rows_dynamic(float *__restrict__ A, const float *__restrict__ B,
                              size_t N1d) {
    const float s = 1.0001f;
    const size_t chunk = std::max<size_t>(1, N1d / (size_t)(4 * omp_get_max_threads()));
    #pragma omp parallel for schedule(dynamic, 1) firstprivate(chunk)
    for (size_t cy = 0; cy < N1d; cy += chunk) {
        size_t cyhi = std::min(cy + chunk, N1d);
        for (size_t y = cy; y < cyhi; y++) {
            size_t row = y * N1d;
            #pragma omp simd
            for (size_t x = 0; x < N1d; x++) {
                size_t i = row + x;
                A[i] = s * A[i] + B[i];
            }
        }
    }
}

/*
 * 2x2 NUMA-aware tiled kernel with inner cache-blocking.
 * Each thread stays inside the quadrant it first-touched, then sweeps
 * its row-slice of the quadrant in TILE_Y x TILE_X tiles.
 */
static void k_2x2_numa_tiled(float *__restrict__ A, const float *__restrict__ B,
                             size_t N1d, size_t TILE_Y, size_t TILE_X) {
    const float s = 1.0001f;
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int P   = omp_get_num_threads();
        int Pq  = P / 4;
        int q   = tid / Pq;
        int lid = tid % Pq;
        int qy  = q / 2;
        int qx  = q % 2;

        size_t H   = N1d / 2;
        size_t y0  = (size_t)qy * H;
        size_t x0  = (size_t)qx * H;
        size_t y1  = y0 + H;
        size_t x1  = x0 + H;

        size_t rows_per = H / Pq;
        size_t ly0 = y0 + (size_t)lid * rows_per;
        size_t ly1 = (lid == Pq - 1) ? y1 : ly0 + rows_per;

        for (size_t ty = ly0; ty < ly1; ty += TILE_Y) {
            size_t tyhi = std::min(ty + TILE_Y, ly1);
            for (size_t tx = x0; tx < x1; tx += TILE_X) {
                size_t txhi = std::min(tx + TILE_X, x1);
                for (size_t y = ty; y < tyhi; y++) {
                    size_t row = y * N1d;
                    #pragma omp simd
                    for (size_t x = tx; x < txhi; x++) {
                        size_t i = row + x;
                        A[i] = s * A[i] + B[i];
                    }
                }
            }
        }
    }
}

/* ── harness ──────────────────────────────────────────────────────── */

struct IterRecord {
    std::string variant;
    size_t TILE_Y, TILE_X;
    int rep, nthreads;
    double time_s, bw_gbs;
    double checksum;
    const char *status;
    bool flush;
};

static std::vector<IterRecord> g_records;

static double reduce_sum(const float *__restrict__ A, size_t N) {
    double cs = 0.0;
    #pragma omp parallel for reduction(+:cs) schedule(static)
    for (size_t i = 0; i < N; i++) cs += (double)A[i];
    return cs;
}

/* Run NWARMUP + NREP iterations of `run()`. When use_flush is true we
 * call flush_jacobi() between every iteration (cold-cache peak). When
 * false, no flush is issued and successive reps see warm caches -- this
 * matches the old common/bench_stream.cpp "peak" methodology.
 * `run()` applies the scale-add in-place to A. Stores per-rep records
 * in g_records; the headline print is the arithmetic mean across reps. */
template <typename Run>
static void bench_variant(const char *variant, size_t TY, size_t TX,
                          Run &&run, float *A, float *B,
                          size_t N1d, int nthreads,
                          double ref_checksum_after_one,
                          bool use_flush) {
    const size_t N = N1d * N1d;

    /* correctness: re-init A (and B for symmetry) then run once. */
    first_touch_2x2(A, N1d, 1.0f);
    first_touch_2x2(B, N1d, 2.0f);
    run();
    double cs = reduce_sum(A, N);
    bool ok = std::fabs(cs - ref_checksum_after_one) <= 1e-3 * std::fabs(ref_checksum_after_one);
    const char *status = ok ? "PASS" : "FAIL";
    if (!ok) {
        fprintf(stderr, "  [%s TY=%zu TX=%zu flush=%s] CHECKSUM MISMATCH: got %.6e exp %.6e\n",
                variant, TY, TX, use_flush ? "yes" : "no", cs, ref_checksum_after_one);
    }

    /* warmups */
    for (int w = 0; w < NWARMUP; w++) {
        if (use_flush) flush_jacobi();
        run();
    }

    /* timed reps. Re-init not needed for bandwidth: the RMW moves the
     * same bytes regardless of A's value. */
    const double data_bytes = 3.0 * (double)N * sizeof(float); /* 2 loads + 1 store */
    for (int r = 0; r < NREP; r++) {
        if (use_flush) flush_jacobi();
        auto t0 = Clock::now();
        run();
        auto t1 = Clock::now();
        double dt = elapsed_sec(t0, t1);
        double bw = data_bytes / dt / 1e9;
        g_records.push_back({variant, TY, TX, r, nthreads, dt, bw, cs, status, use_flush});
    }

    /* brief stdout summary (mean + range) */
    std::vector<double> bws;
    for (int i = (int)g_records.size() - NREP; i < (int)g_records.size(); i++)
        bws.push_back(g_records[i].bw_gbs);
    double sum = 0.0; for (double b : bws) sum += b;
    double mean = sum / (double)NREP;
    std::sort(bws.begin(), bws.end());
    printf("  %-22s TY=%-5zu TX=%-5zu flush=%-3s mean %7.1f GB/s  [%7.1f .. %7.1f]  %s\n",
           variant, TY, TX, use_flush ? "yes" : "no",
           mean, bws.front(), bws.back(), status);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
                "Usage: %s <csv_file> [N1d=16384] [nthreads=0=env]\n", argv[0]);
        return 1;
    }
    const char *csv_path = argv[1];
    size_t N1d          = (argc > 2) ? (size_t)atoll(argv[2]) : 16384;
    int    req_threads  = (argc > 3) ? atoi(argv[3])          : 0;
    if (req_threads > 0) omp_set_num_threads(req_threads);

    int nthreads = 0;
    #pragma omp parallel
    { nthreads = omp_get_num_threads(); }

    if (nthreads % 4 != 0) {
        fprintf(stderr, "[bench_stream cpu] WARNING: nthreads=%d not a multiple of 4; "
                        "2x2 NUMA split will have uneven groups.\n", nthreads);
    }
    if (N1d % 2 != 0) {
        fprintf(stderr, "[bench_stream cpu] ERROR: N1d=%zu must be even.\n", N1d);
        return 1;
    }

    const size_t N     = N1d * N1d;
    const size_t bytes = N * sizeof(float);

    printf("NUMA STREAM peak (E0)\n");
    printf("  N1d=%zu  N=%zu  (%.2f GiB/buf)  threads=%d  NUMA nodes=%d\n",
           N1d, N, bytes / (double)(1UL << 30), nthreads, numa_num_nodes());
    printf("  reps=%d  warmup=%d  stat=arithmetic mean\n", NREP, NWARMUP);
    printf("  two passes: flush=yes (cold cache, flush_jacobi between reps)\n");
    printf("              flush=no  (warm cache, legacy peak methodology)\n\n");

    /* ── allocate + NUMA first-touch (2x2 quadrants) ─────────────────── */
    float *A = numa_alloc<float>(N);
    float *B = numa_alloc<float>(N);
    first_touch_2x2(A, N1d, 1.0f);
    first_touch_2x2(B, N1d, 2.0f);

    /* ── reference checksum: run scale-add once, reduce A ────────────── */
    /* A was just first-touched to 1.0f, so one RMW gives:
     *   A[i] = 1.0001 * 1.0 + 2.0 = 3.0001 for every i.
     * That's our analytic expected value independent of the variant. */
    const double ref_cs = 3.0001 * (double)N;

    const size_t TILE_Ys[] = {16, 32, 64, 128, 256, 512};
    const size_t TILE_Xs[] = {64, 128, 256, 512, 1024, 2048, 4096, 8192};

    for (bool use_flush : { true, false }) {
        printf("\n######## flush=%s ########\n", use_flush ? "yes" : "no");

        /* ── 1D baselines ─────────────────────────────────────────────── */
        printf("=== 1D default schedules ===\n");
        bench_variant("1d_rows_static",     0, 0,
                      [&]{ k_1d_rows_static(A, B, N1d); },
                      A, B, N1d, nthreads, ref_cs, use_flush);
        bench_variant("1d_collapse_static", 0, 0,
                      [&]{ k_1d_collapse_static(A, B, N1d); },
                      A, B, N1d, nthreads, ref_cs, use_flush);
        bench_variant("1d_flat_static",     0, 0,
                      [&]{ k_1d_flat_static(A, B, N1d); },
                      A, B, N1d, nthreads, ref_cs, use_flush);
        bench_variant("1d_rows_dynamic",    0, 0,
                      [&]{ k_1d_rows_dynamic(A, B, N1d); },
                      A, B, N1d, nthreads, ref_cs, use_flush);

        /* ── 2x2 NUMA-aware tiled sweep ──────────────────────────────── */
        printf("\n=== 2x2 NUMA tiled (inner TILE_Y x TILE_X sweep) ===\n");
        for (size_t TY : TILE_Ys)
            for (size_t TX : TILE_Xs) {
                bench_variant("2x2_numa_tiled", TY, TX,
                              [&]{ k_2x2_numa_tiled(A, B, N1d, TY, TX); },
                              A, B, N1d, nthreads, ref_cs, use_flush);
            }
    }

    /* ── write per-iteration CSV ──────────────────────────────────────── */
    FILE *fp = fopen(csv_path, "w");
    if (!fp) { perror(csv_path); return 1; }
    fprintf(fp, "variant,TILE_Y,TILE_X,N,rep,threads,time_s,bw_gbs,checksum,status,flush\n");
    for (const auto &r : g_records)
        fprintf(fp, "%s,%zu,%zu,%zu,%d,%d,%.9f,%.3f,%.6e,%s,%s\n",
                r.variant.c_str(), r.TILE_Y, r.TILE_X, N,
                r.rep, r.nthreads, r.time_s, r.bw_gbs, r.checksum, r.status,
                r.flush ? "yes" : "no");
    fclose(fp);
    printf("\nWrote %zu records to %s\n", g_records.size(), csv_path);

    numa_dealloc(A, N);
    numa_dealloc(B, N);
    return 0;
}
