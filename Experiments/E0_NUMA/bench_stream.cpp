/*
 * bench_stream.cpp -- CPU STREAM sweep (scale-add RMW) with explicit
 * NUMA setup variants. fp32 matrix N1d x N1d; kernel is
 *     A[y,x] = s * A[y,x] + B[y,x]
 * Traffic: 2 loads + 1 store per fp32 = 12 bytes per element.
 *
 * The experiment is structured as setup x kernel x flush:
 *
 *   setup:    ft_flat       -- plain 1D first-touch via omp for schedule(static).
 *                              Pages land on whatever NUMA node the first-
 *                              touching thread is bound to. Hugepage-clean
 *                              because each thread writes a contiguous
 *                              elements stripe = contiguous page range.
 *             bind_stripe_4 -- explicit mbind into 4 row-stripes (pages
 *                              0..N/4-1 -> node 0, ... ). Uses MPOL_MF_MOVE
 *                              so pages placed by a previous setup are
 *                              migrated; this is the canonical NUMA-tiling
 *                              the paper refers to.
 *             interleave    -- MPOL_INTERLEAVE across all nodes (page-
 *                              cyclic). Kept as a comparison point.
 *
 *   kernel:   1d_rows_static        -- omp for schedule(static) over rows
 *             1d_rows_dynamic       -- omp for schedule(dynamic) over rows
 *             1d_flat_static        -- manual flat 1D element partition
 *                                      (matches common/bench_stream.cpp)
 *             4_stripe_aware        -- manual partition in which thread
 *                                      tid = g*24 + l handles the l-th
 *                                      sub-slice of the g-th NUMA stripe.
 *                                      Aligned with bind_stripe_4.
 *
 *   flush:    yes / no
 *
 * After each setup variant is applied, numa_report_distribution() is
 * called on both A and B so the actual page placement can be verified
 * from the run log -- no guessing about whether mbind did what we asked.
 *
 * Output CSV (one row per repetition):
 *   setup,variant,N,rep,threads,time_s,bw_gbs,checksum,status,flush
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

/* ── setup helpers ─────────────────────────────────────────────────────
 *
 * Each setup modifies the page-placement policy of an already-allocated
 * buffer and re-touches every page to realize it. Because numa_util's
 * bind_and_touch / interleave_and_touch pass MPOL_MF_MOVE, the buffers
 * can be reused across setups without a munmap / re-mmap.
 *
 * ft_flat populates A/B via plain omp for schedule(static), which lands
 * each page on whichever NUMA node the thread doing the first write was
 * bound to. With OMP_PLACES={0}:24:1, {24}:24:1, ... this produces a
 * nearly-uniform 4-node distribution, with only thread-boundary pages
 * potentially landing off-target.
 */
static void setup_ft_flat(float *__restrict__ A, float *__restrict__ B,
                          size_t N, float va, float vb) {
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < N; ++i) { A[i] = va; B[i] = vb; }
}

static void setup_bind_stripe_4(float *__restrict__ A, float *__restrict__ B,
                                size_t N, float va, float vb) {
    bind_and_touch(A, N * sizeof(float));
    bind_and_touch(B, N * sizeof(float));
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < N; ++i) { A[i] = va; B[i] = vb; }
}

static void setup_interleave(float *__restrict__ A, float *__restrict__ B,
                             size_t N, float va, float vb) {
    interleave_and_touch(A, N * sizeof(float));
    interleave_and_touch(B, N * sizeof(float));
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < N; ++i) { A[i] = va; B[i] = vb; }
}

/* ── kernels ────────────────────────────────────────────────────────── */

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

/*
 * 4-stripe NUMA-aware partition: with OMP_PLACES arranged as four
 * 24-thread groups each pinned to one NUMA node, thread tid splits into
 *     group g = tid / 24
 *     local l = tid % 24
 * and handles the l-th sub-slice of the g-th N/4-element stripe. This
 * aligns exactly with bind_stripe_4 (both use N/4-byte NUMA stripes),
 * so every load and store is NUMA-local.
 */
static void k_4_stripe_aware(float *__restrict__ A, const float *__restrict__ B,
                             size_t N1d) {
    const float s = 1.0001f;
    const size_t N = N1d * N1d;
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int P   = omp_get_num_threads();
        int Pq  = P / 4;                   /* threads per NUMA group   */
        int g   = tid / Pq;                /* NUMA group id 0..3       */
        int l   = tid % Pq;                /* local thread in group    */
        size_t stripe_N = N / 4;
        size_t base = (size_t)g * stripe_N;
        size_t lo = base + (size_t)l * stripe_N / (size_t)Pq;
        size_t hi = base + (size_t)(l + 1) * stripe_N / (size_t)Pq;
        #pragma omp simd
        for (size_t i = lo; i < hi; i++) A[i] = s * A[i] + B[i];
    }
}

/* ── harness ──────────────────────────────────────────────────────── */

struct IterRecord {
    std::string setup, variant;
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

/* Run NWARMUP + NREP iterations of `run()` and record per-rep BW. When
 * use_flush is true, a canonical 8192^2 Jacobi flush runs between every
 * rep; otherwise the caches and prefetchers carry over. `reinit` is
 * called once before the correctness check so A/B are in a known state.
 * The correctness check uses the analytic invariant:
 *     A[i] = s*1.0 + 2.0  = 3.0001 for every i after one RMW pass. */
template <typename ReinitFn, typename RunFn>
static void bench_variant(const char *setup, const char *variant,
                          ReinitFn &&reinit, RunFn &&run,
                          float *A, float *B, size_t N1d, int nthreads,
                          double ref_checksum_after_one, bool use_flush) {
    const size_t N = N1d * N1d;

    /* correctness: re-init A and B, run once, reduce A. */
    reinit();
    run();
    double cs = reduce_sum(A, N);
    bool ok = std::fabs(cs - ref_checksum_after_one)
              <= 1e-3 * std::fabs(ref_checksum_after_one);
    const char *status = ok ? "PASS" : "FAIL";
    if (!ok) {
        fprintf(stderr,
                "  [%s/%s flush=%s] CHECKSUM MISMATCH: got %.6e exp %.6e\n",
                setup, variant, use_flush ? "yes" : "no",
                cs, ref_checksum_after_one);
    }

    /* warmups */
    for (int w = 0; w < NWARMUP; w++) {
        if (use_flush) flush_jacobi();
        run();
    }

    /* timed reps. Re-init not needed for bandwidth: the RMW moves the
     * same bytes regardless of A's value. */
    const double data_bytes = 3.0 * (double)N * sizeof(float);
    for (int r = 0; r < NREP; r++) {
        if (use_flush) flush_jacobi();
        auto t0 = Clock::now();
        run();
        auto t1 = Clock::now();
        double dt = elapsed_sec(t0, t1);
        double bw = data_bytes / dt / 1e9;
        g_records.push_back({setup, variant, r, nthreads, dt, bw, cs,
                             status, use_flush});
    }

    /* brief stdout summary (min = peak, mean, max) */
    std::vector<double> bws;
    for (int i = (int)g_records.size() - NREP; i < (int)g_records.size(); i++)
        bws.push_back(g_records[i].bw_gbs);
    double sum = 0.0; for (double b : bws) sum += b;
    double mean = sum / (double)NREP;
    std::sort(bws.begin(), bws.end());
    printf("  %-14s %-18s flush=%-3s  min %7.1f  mean %7.1f  max %7.1f GB/s  %s\n",
           setup, variant, use_flush ? "yes" : "no",
           bws.front(), mean, bws.back(), status);
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
                        "4_stripe_aware kernel will have uneven groups.\n", nthreads);
    }

    const size_t N     = N1d * N1d;
    const size_t bytes = N * sizeof(float);

    printf("NUMA STREAM peak (E0)\n");
    printf("  N1d=%zu  N=%zu  (%.2f GiB/buf)  threads=%d  NUMA nodes=%d\n",
           N1d, N, bytes / (double)(1UL << 30), nthreads, numa_num_nodes());
    printf("  reps=%d  warmup=%d  headline stat=peak (min time -> max BW)\n",
           NREP, NWARMUP);

    float *A = numa_alloc<float>(N);
    float *B = numa_alloc<float>(N);

    /* A first-touched to 1.0 and B to 2.0 => one RMW gives A[i]=3.0001   */
    /* independent of variant, which makes the reference checksum cheap. */
    const float  VA     = 1.0f, VB = 2.0f;
    const double ref_cs = 3.0001 * (double)N;

    /* Kernel table */
    struct KernelEntry {
        const char *name;
        void (*fn)(float *, const float *, size_t);
    };
    const KernelEntry kernels[] = {
        { "1d_rows_static",   k_1d_rows_static   },
        { "1d_rows_dynamic",  k_1d_rows_dynamic  },
        { "1d_flat_static",   k_1d_flat_static   },
        { "4_stripe_aware",   k_4_stripe_aware   },
    };
    const int NK = (int)(sizeof(kernels) / sizeof(kernels[0]));

    /* Setup table */
    struct SetupEntry {
        const char *name;
        void (*apply)(float *, float *, size_t, float, float);
    };
    const SetupEntry setups[] = {
        { "ft_flat",       setup_ft_flat       },
        { "bind_stripe_4", setup_bind_stripe_4 },
        { "interleave",    setup_interleave    },
    };
    const int NS = (int)(sizeof(setups) / sizeof(setups[0]));

    for (int si = 0; si < NS; si++) {
        const char *setup_name = setups[si].name;
        printf("\n######## setup=%s ########\n", setup_name);

        /* Apply the NUMA placement policy and touch every page. */
        setups[si].apply(A, B, N, VA, VB);

        /* Dump the actual page distribution so a reviewer can see which
         * nodes the pages landed on without guessing.               */
        char lblA[64], lblB[64];
        std::snprintf(lblA, sizeof(lblA), "%s A", setup_name);
        std::snprintf(lblB, sizeof(lblB), "%s B", setup_name);
        numa_report_distribution(A, bytes, lblA);
        numa_report_distribution(B, bytes, lblB);

        for (int ki = 0; ki < NK; ki++) {
            auto kfn = kernels[ki].fn;
            const char *kname = kernels[ki].name;
            auto reinit = [&]{
                /* Re-set A and B to canonical values under the current
                 * placement policy. Uses the same schedule as ft_flat
                 * for simplicity; this does not change page placement
                 * (pages are already bound), only the values.        */
                #pragma omp parallel for schedule(static)
                for (size_t i = 0; i < N; ++i) { A[i] = VA; B[i] = VB; }
            };
            auto run = [&]{ kfn(A, B, N1d); };

            for (bool fl : { true, false }) {
                bench_variant(setup_name, kname, reinit, run,
                              A, B, N1d, nthreads, ref_cs, fl);
            }
        }
    }

    /* ── write per-iteration CSV ──────────────────────────────────── */
    FILE *fp = fopen(csv_path, "w");
    if (!fp) { perror(csv_path); return 1; }
    fprintf(fp, "setup,variant,N,rep,threads,time_s,bw_gbs,checksum,status,flush\n");
    for (const auto &r : g_records)
        fprintf(fp, "%s,%s,%zu,%d,%d,%.9f,%.3f,%.6e,%s,%s\n",
                r.setup.c_str(), r.variant.c_str(), N,
                r.rep, r.nthreads, r.time_s, r.bw_gbs, r.checksum, r.status,
                r.flush ? "yes" : "no");
    fclose(fp);
    printf("\nWrote %zu records to %s\n", g_records.size(), csv_path);

    numa_dealloc(A, N);
    numa_dealloc(B, N);
    return 0;
}
