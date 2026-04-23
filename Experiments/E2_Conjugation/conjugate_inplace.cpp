#include <omp.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <numaif.h>
#include <unistd.h>

#include "../common/jacobi_flush.h"

/*  Conjugate P complex arrays in-place:
 *      buf[i].im_p = -buf[i].im_p      (negate im only)
 *
 *  Total doubles = 2·P·N_base = const across P.
 *  We report useful BW = 2·P·N_base·8 uniformly.
 *
 *  KEY: single persistent omp parallel region for the entire
 *  benchmark loop.  This eliminates thread wake-up jitter across
 *  4 NUMA nodes — the #1 cause of variance on Grace.                */

constexpr int64_t TOTAL_DOUBLES = 1LL << 30;
constexpr int     NRUNS = 100;
constexpr int     NWARM = 5;
constexpr int64_t MAX_VL = 512;

template<int P>
struct AoS {
    struct { double re, im; } c[P];
};

template<int P, int VL>
struct AoSoA {
    struct { double re[VL], im[VL]; } c[P];
};

/* ═══ NUMA helpers ═══
 *
 * Provided by ../common/numa_util.h (included via jacobi_flush.h):
 *   - numa_alloc(bytes), numa_free(p, bytes)
 *   - bind_and_touch(base, total_bytes)
 * Global policy: MADV_HUGEPAGE on every allocation. */

/* ═══ Cache flush ═══
 *
 * Canonical Jacobi-2D cache flush shared across all experiments
 * (see ../common/jacobi_flush.h). 8192x8192, 3 swept Jacobi sweeps,
 * buffers allocated once at init and NUMA-spread via first-touch.
 * The _inner variant is used from inside the persistent parallel
 * region — flush_init() must be called from the serial driver first. */
static void flush_init()  { flush_jacobi_init(); }
static void flush_free()  {}
static void flush_inner() { flush_jacobi_inner(); }

/* ═══ Init helpers ═══ */

static void init_and_bind(double *__restrict__ buf, int64_t n) {
    bind_and_touch(buf, n * sizeof(double));
    #pragma omp parallel for schedule(static)
    for (int64_t i = 0; i < n; i++)
        buf[i] = (double)(i % 997) * 0.001;
}

template<int P>
static void init_aos(AoS<P> *buf, int64_t n) {
    bind_and_touch(buf, n * sizeof(AoS<P>));
    #pragma omp parallel for schedule(static)
    for (int64_t i = 0; i < n; i++)
        for (int p = 0; p < P; p++) {
            buf[i].c[p].re = (double)((i * P + p) % 997) * 0.001;
            buf[i].c[p].im = (double)((i * P + p) % 991) * 0.001;
        }
}

template<int P, int VL>
static void init_aosoa(AoSoA<P,VL> *buf, int64_t n_base) {
    int64_t nblks = n_base / VL;
    bind_and_touch(buf, nblks * sizeof(AoSoA<P,VL>));
    #pragma omp parallel for schedule(static)
    for (int64_t b = 0; b < nblks; b++)
        for (int p = 0; p < P; p++)
            for (int l = 0; l < VL; l++) {
                int64_t idx = b * VL + l;
                buf[b].c[p].re[l] = (double)((idx * P + p) % 997) * 0.001;
                buf[b].c[p].im[l] = (double)((idx * P + p) % 991) * 0.001;
            }
}

/* ═══ Kernels — "inner" versions for use inside an existing parallel region ═══
 *
 * These use  #pragma omp for  (worksharing, no new parallel region)
 * instead of #pragma omp parallel for.                               */

template<int P>
static void kern_aos_inner(AoS<P> *__restrict__ buf, int64_t n) {
    #pragma omp for schedule(static)
    for (int64_t i = 0; i < n; i++)
        for (int p = 0; p < P; p++)
            buf[i].c[p].im = -buf[i].c[p].im;
    /* implicit barrier */
}

template<int P>
static void kern_soa_inner(double *const *__restrict__ im, int64_t n) {
    #pragma omp for schedule(static)
    for (int64_t i = 0; i < n; i++)
        for (int p = 0; p < P; p++)
            im[p][i] = -im[p][i];
}

template<int P, int VL>
static void kern_aosoa_inner(AoSoA<P,VL> *__restrict__ buf, int64_t n_base) {
    int64_t nblks = n_base / VL;
    #pragma omp for schedule(static)
    for (int64_t b = 0; b < nblks; b++)
        for (int p = 0; p < P; p++)
            #pragma omp simd
            for (int l = 0; l < VL; l++)
                buf[b].c[p].im[l] = -buf[b].c[p].im[l];
}

/* ═══ Numerical verification (in-place) ═══
 *
 * Contract: after one conjugate call, buf[i].c[p].im == -original_im
 * and buf[i].c[p].re is unchanged. Kernel has no floating-point
 * rounding (pure unary negation), so we compare with tolerance 0.
 * verify_* runs the kernel exactly once on a fresh buffer and compares
 * against the init-time formula with sign flipped. Called once from
 * each bench function before the timed loop. */

template<int P>
static bool verify_aos_inplace(int64_t n) {
    size_t bytes = n * sizeof(AoS<P>);
    auto *buf = (AoS<P> *)numa_alloc(bytes); init_aos<P>(buf, n);
    #pragma omp parallel
    { kern_aos_inner<P>(buf, n); }
    long long mism = 0;
    #pragma omp parallel for schedule(static) reduction(+:mism)
    for (int64_t i = 0; i < n; i++)
        for (int p = 0; p < P; p++) {
            double ref_re = (double)((i * P + p) % 997) * 0.001;
            double ref_im = (double)((i * P + p) % 991) * 0.001;
            if (buf[i].c[p].re != ref_re)   mism++;
            if (buf[i].c[p].im != -ref_im)  mism++;
        }
    numa_free(buf, bytes);
    if (mism) fprintf(stderr, "[verify][FAIL] AoS-inplace<P=%d>  mismatches=%lld\n", P, mism);
    else      fprintf(stderr, "[verify][OK]   AoS-inplace<P=%d>\n", P);
    return mism == 0;
}

template<int P>
static bool verify_soa_inplace(int64_t n) {
    double *im[P];
    for (int p = 0; p < P; p++) {
        im[p] = numa_alloc<double>(n);
        #pragma omp parallel for schedule(static)
        for (int64_t i = 0; i < n; i++)
            im[p][i] = (double)((i * P + p) % 991) * 0.001;
    }
    #pragma omp parallel
    { kern_soa_inner<P>(im, n); }
    long long mism = 0;
    for (int p = 0; p < P; p++) {
        #pragma omp parallel for schedule(static) reduction(+:mism)
        for (int64_t i = 0; i < n; i++) {
            double ref_im = (double)((i * P + p) % 991) * 0.001;
            if (im[p][i] != -ref_im) mism++;
        }
    }
    for (int p = 0; p < P; p++) numa_dealloc(im[p], n);
    if (mism) fprintf(stderr, "[verify][FAIL] SoA-inplace<P=%d>  mismatches=%lld\n", P, mism);
    else      fprintf(stderr, "[verify][OK]   SoA-inplace<P=%d>\n", P);
    return mism == 0;
}

template<int P, int VL>
static bool verify_aosoa_inplace(int64_t n, const char *label) {
    int64_t nblks = n / VL;
    size_t bytes = nblks * sizeof(AoSoA<P,VL>);
    auto *buf = (AoSoA<P,VL> *)numa_alloc(bytes); init_aosoa<P,VL>(buf, n);
    #pragma omp parallel
    { kern_aosoa_inner<P,VL>(buf, n); }
    long long mism = 0;
    #pragma omp parallel for schedule(static) reduction(+:mism)
    for (int64_t b = 0; b < nblks; b++)
        for (int p = 0; p < P; p++)
            for (int l = 0; l < VL; l++) {
                int64_t idx = b * VL + l;
                double ref_re = (double)((idx * P + p) % 997) * 0.001;
                double ref_im = (double)((idx * P + p) % 991) * 0.001;
                if (buf[b].c[p].re[l] != ref_re)  mism++;
                if (buf[b].c[p].im[l] != -ref_im) mism++;
            }
    numa_free(buf, bytes);
    if (mism) fprintf(stderr, "[verify][FAIL] %s-inplace<P=%d>  mismatches=%lld\n", label, P, mism);
    else      fprintf(stderr, "[verify][OK]   %s-inplace<P=%d>\n", label, P);
    return mism == 0;
}

/* ═══ Bench driver ═══
 *
 * Single persistent parallel region: flush + barrier + time + kernel
 * all happen with threads already alive.  No thread wake-up jitter.  */

static FILE *csv;

static double bw_ip(int P, int64_t n_base, double ms) {
    return 2.0 * P * n_base * sizeof(double) / (ms * 1e6);
}

/* Run flush+kernel inside one persistent parallel region.
 * Fn must be a callable that uses #pragma omp for (not parallel for). */
template<typename Fn>
static void bench_persistent(int P, int64_t n_base, const char *label, Fn &&kernel) {
    double times[NRUNS];

    #pragma omp parallel
    {
        /* warmup — all inside this parallel region */
        for (int w = 0; w < NWARM; w++) {
            flush_inner();
            kernel();       /* uses omp for inside */
        }

        /* timed runs */
        for (int r = 0; r < NRUNS; r++) {
            flush_inner();          /* omp for + implicit barrier */

            /* master records start time; barrier ensures all see it */
            double t0;
            #pragma omp barrier
            #pragma omp master
            { t0 = omp_get_wtime(); }
            #pragma omp barrier     /* all threads start kernel together */

            kernel();               /* omp for + implicit barrier */

            #pragma omp master
            { times[r] = (omp_get_wtime() - t0) * 1e3; }
            #pragma omp barrier     /* wait for master to record */
        }
    }
    /* parallel region ends — now single-threaded */

    double sum = 0;
    for (int r = 0; r < NRUNS; r++) {
        double gbs = bw_ip(P, n_base, times[r]);
        fprintf(csv, "%d,%s,%d,%.6f,%.2f\n", P, label, r, times[r], gbs);
        sum += times[r];
    }
    double avg = sum / NRUNS;
    printf("  %-14s %8.4f ms  %7.1f GB/s\n",
           label, avg, bw_ip(P, n_base, avg));
}

/* ═══ Per-layout wrappers ═══ */

template<int P>
static void bench_aos(int64_t n) {
    verify_aos_inplace<P>(n < 1<<16 ? n : 1<<16);
    size_t bytes = n * sizeof(AoS<P>);
    auto *buf = (AoS<P> *)numa_alloc(bytes); init_aos<P>(buf, n);
    bench_persistent(P, n, "AoS",
        [=]() { kern_aos_inner<P>(buf, n); });
    numa_free(buf, bytes);
}

template<int P>
static void bench_soa(int64_t n) {
    verify_soa_inplace<P>(n < 1<<16 ? n : 1<<16);
    size_t bytes = n * sizeof(double);
    size_t pad   = (size_t)P * 64;          /* per-array cache-line offset budget */
    double *re[P], *im[P];
    void   *im_raw[P];                      /* raw mmap pointers for munmap */

    for (int p = 0; p < P; p++) {
        re[p]     = (double *)numa_alloc(bytes);
        init_and_bind(re[p], n);
        im_raw[p] = numa_alloc(bytes + pad);
        im[p]     = (double *)((char *)im_raw[p] + (size_t)p * 64);
        init_and_bind(im[p], n);
    }

    double *const *ip = im;
    bench_persistent(P, n, "SoA",
            [=]() { kern_soa_inner<P>(ip, n); });

    for (int p = 0; p < P; p++) {
        numa_free(re[p], bytes);
        numa_free(im_raw[p], bytes + pad);
    }
}

template<int P, int VL>
static void bench_aosoa(int64_t n, const char *label) {
    int64_t nverif = n < 1<<16 ? n : 1<<16;
    nverif = (nverif / VL) * VL;           /* align to VL */
    if (nverif > 0) verify_aosoa_inplace<P, VL>(nverif, label);
    int64_t nblks = n / VL;
    size_t bytes = nblks * sizeof(AoSoA<P,VL>);
    auto *buf = (AoSoA<P,VL> *)numa_alloc(bytes); init_aosoa<P,VL>(buf, n);
    bench_persistent(P, n, label,
        [=]() { kern_aosoa_inner<P,VL>(buf, n); });
    numa_free(buf, bytes);
}

template<int P>
static void run_all() {
    int64_t n = (TOTAL_DOUBLES / (2 * P) / MAX_VL) * MAX_VL;
    printf("\n── P=%d complex pairs  (%d im streams IP)  N_base=%lld  "
           "total=%.1f GB ──\n",
           P, P, (long long)n, 2.0 * P * n * 8 / 1e9);

    bench_aos<P>(n);
    bench_soa<P>(n);
    bench_aosoa<P,   2>(n, "AoSoA-2");
    bench_aosoa<P,   4>(n, "AoSoA-4");
    bench_aosoa<P,   8>(n, "AoSoA-8");
    bench_aosoa<P,  16>(n, "AoSoA-16");
    bench_aosoa<P,  32>(n, "AoSoA-32");
    bench_aosoa<P,  64>(n, "AoSoA-64");
    bench_aosoa<P, 128>(n, "AoSoA-128");
    bench_aosoa<P, 256>(n, "AoSoA-256");
    bench_aosoa<P, 512>(n, "AoSoA-512");
}

int main(int argc, char **argv) {
    const char *csv_path = (argc > 1) ? argv[1] : "results_cpu_inplace.csv";
    flush_init();

    csv = fopen(csv_path, "w");
    fprintf(csv, "P,layout,run,ms,gbps\n");

    printf("conjugate IN-PLACE (CPU): TOTAL_DOUBLES=%lldM  runs=%d  threads=%d\n",
           (long long)(TOTAL_DOUBLES >> 20), NRUNS, omp_get_max_threads());
    printf("BW = 2·P·N_base·8  (read P im + write P im)\n");
    printf("Mode: persistent parallel region (no thread wake-up jitter)\n");

    {
        int nodes[8] = {};
        #pragma omp parallel
        { int nd = get_numa_node(); if (nd >= 0 && nd < 8) {
            #pragma omp atomic
            nodes[nd]++;
        }}
        printf("  thread spread:");
        for (int i = 0; i < 8; i++) if (nodes[i]) printf(" N%d=%d", i, nodes[i]);
        int n_active = 0;
        for (int i = 0; i < 8; i++) if (nodes[i]) n_active++;
        printf("  (%d NUMA nodes)\n", n_active);
    }

    run_all< 3>();
    run_all< 6>();
    run_all< 9>();
    run_all<12>();
    run_all<15>();
    run_all<18>();
    run_all<21>();

    fclose(csv);
    flush_free();
    printf("\nwrote results_cpu_inplace.csv\n");
}