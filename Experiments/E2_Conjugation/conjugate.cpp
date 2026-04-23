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

/*  Conjugate P complex arrays out-of-place:
 *      out[i].re_p =  in[i].re_p       (copy)
 *      out[i].im_p = -in[i].im_p       (negate)
 *
 *  Total doubles = 2·P·N_base = const  ⇒  same footprint for every P.
 *  OOP BW = 2 × (2·P·N_base) × 8   (read all + write all).
 *  SoA has 4·P distinct streams (2P in + 2P out).                    */

constexpr int64_t TOTAL_DOUBLES = 1LL << 30;   // ~2 GB per side
constexpr int64_t RUNS  = 100;
constexpr int64_t MAX_VL = 512;

/* ═══ Layout structs ═══ */

template<int P>
struct AoS {
    struct { double re, im; } c[P];   /* P interleaved (re,im) pairs */
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
 * buffers allocated once at init and NUMA-spread via first-touch. */
static void flush_init()    { flush_jacobi_init(); }
static void flush_caches()  { flush_jacobi(); }
static void flush_free()    {}

/* ═══ Init helpers ═══ */

static void init_and_bind(double *buf, int64_t n) {
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

/* ═══ Kernels (out-of-place): copy re, negate im ═══ */

template<int P>
static void kern_aos(const AoS<P> *__restrict__ in,
                     AoS<P> *__restrict__ out, int64_t n) {
    #pragma omp parallel for schedule(static)
    for (int64_t i = 0; i < n; i++)
        for (int p = 0; p < P; p++) {
            out[i].c[p].re =  in[i].c[p].re;
            out[i].c[p].im = -in[i].c[p].im;
        }
}

template<int P>
static void kern_soa(double *const *__restrict__ re_in,
                     double *const *__restrict__ im_in,
                     double *const *__restrict__ re_out,
                     double *const *__restrict__ im_out,
                     int64_t n) {
    #pragma omp parallel for schedule(static)
    for (int64_t i = 0; i < n; i++)
        for (int p = 0; p < P; p++) {
            re_out[p][i] =  re_in[p][i];
            im_out[p][i] = -im_in[p][i];
        }
}

template<int P, int VL>
static void kern_aosoa(const AoSoA<P,VL> *__restrict__ in,
                       AoSoA<P,VL> *__restrict__ out, int64_t n_base) {
    int64_t nblks = n_base / VL;
    #pragma omp parallel for schedule(static)
    for (int64_t b = 0; b < nblks; b++)
        for (int p = 0; p < P; p++) {
            #pragma omp simd
            for (int l = 0; l < VL; l++) {
                out[b].c[p].re[l] =  in[b].c[p].re[l];
                out[b].c[p].im[l] = -in[b].c[p].im[l];
            }
        }
}

/* ═══ Numerical verification (out-of-place) ═══
 *
 * The kernel contract is exact: out.re = in.re, out.im = -in.im. No
 * floating-point rounding is introduced (only copy + unary negation),
 * so we compare with tolerance 0. verify_* runs the kernel once on a
 * freshly initialized buffer pair, then checks every element against
 * the init-time formula and reports pass/fail. Called before each
 * benchmark's timed loop. Returns true iff every element matches. */

template<int P>
static bool verify_aos(int64_t n) {
    size_t bytes = n * sizeof(AoS<P>);
    auto *in  = (AoS<P> *)numa_alloc(bytes); init_aos<P>(in, n);
    auto *out = (AoS<P> *)numa_alloc(bytes); init_aos<P>(out, n);
    kern_aos<P>(in, out, n);
    long long mism = 0;
    #pragma omp parallel for schedule(static) reduction(+:mism)
    for (int64_t i = 0; i < n; i++)
        for (int p = 0; p < P; p++) {
            double er = out[i].c[p].re - in[i].c[p].re;
            double ei = out[i].c[p].im - (-in[i].c[p].im);
            if (er != 0.0 || ei != 0.0) mism++;
        }
    numa_free(in, bytes); numa_free(out, bytes);
    if (mism) fprintf(stderr, "[verify][FAIL] AoS<P=%d>  mismatches=%lld\n", P, mism);
    else      fprintf(stderr, "[verify][OK]   AoS<P=%d>\n", P);
    return mism == 0;
}

template<int P>
static bool verify_soa(int64_t n) {
    /* Minimal allocation: one plain double[n] per stream, no mbind pad. */
    double *re_in[P], *im_in[P], *re_out[P], *im_out[P];
    for (int p = 0; p < P; p++) {
        re_in[p]  = numa_alloc<double>(n);
        im_in[p]  = numa_alloc<double>(n);
        re_out[p] = numa_alloc<double>(n);
        im_out[p] = numa_alloc<double>(n);
        #pragma omp parallel for schedule(static)
        for (int64_t i = 0; i < n; i++) {
            re_in[p][i] = (double)((i * P + p) % 997) * 0.001;
            im_in[p][i] = (double)((i * P + p) % 991) * 0.001;
        }
    }
    kern_soa<P>(re_in, im_in, re_out, im_out, n);
    long long mism = 0;
    for (int p = 0; p < P; p++) {
        #pragma omp parallel for schedule(static) reduction(+:mism)
        for (int64_t i = 0; i < n; i++) {
            if (re_out[p][i] != re_in[p][i])      mism++;
            if (im_out[p][i] != -im_in[p][i])     mism++;
        }
    }
    for (int p = 0; p < P; p++) {
        numa_dealloc(re_in[p], n); numa_dealloc(im_in[p], n);
        numa_dealloc(re_out[p], n); numa_dealloc(im_out[p], n);
    }
    if (mism) fprintf(stderr, "[verify][FAIL] SoA<P=%d>  mismatches=%lld\n", P, mism);
    else      fprintf(stderr, "[verify][OK]   SoA<P=%d>\n", P);
    return mism == 0;
}

template<int P, int VL>
static bool verify_aosoa(int64_t n, const char *label) {
    int64_t nblks = n / VL;
    size_t bytes = nblks * sizeof(AoSoA<P,VL>);
    auto *in  = (AoSoA<P,VL> *)numa_alloc(bytes); init_aosoa<P,VL>(in, n);
    auto *out = (AoSoA<P,VL> *)numa_alloc(bytes); init_aosoa<P,VL>(out, n);
    kern_aosoa<P,VL>(in, out, n);
    long long mism = 0;
    #pragma omp parallel for schedule(static) reduction(+:mism)
    for (int64_t b = 0; b < nblks; b++)
        for (int p = 0; p < P; p++)
            for (int l = 0; l < VL; l++) {
                double er = out[b].c[p].re[l] - in[b].c[p].re[l];
                double ei = out[b].c[p].im[l] - (-in[b].c[p].im[l]);
                if (er != 0.0 || ei != 0.0) mism++;
            }
    numa_free(in, bytes); numa_free(out, bytes);
    if (mism) fprintf(stderr, "[verify][FAIL] %s<P=%d>  mismatches=%lld\n", label, P, mism);
    else      fprintf(stderr, "[verify][OK]   %s<P=%d>\n", label, P);
    return mism == 0;
}

/* ═══ Bench driver ═══ */

static FILE *csv;

static double bw_oop(int P, int64_t n_base, double ms) {
    return 4.0 * P * n_base * sizeof(double) / (ms * 1e6);
}

#define CPU_BENCH(P_val, n_base, label, call) do { \
    for (int w = 0; w < 5; w++) { flush_caches(); call; } \
    double times[RUNS]; \
    for (int r = 0; r < RUNS; r++) { \
        flush_caches(); \
        double t0 = omp_get_wtime(); \
        call; \
        times[r] = (omp_get_wtime() - t0) * 1e3; \
    } \
    double sum = 0; \
    for (int r = 0; r < RUNS; r++) { \
        fprintf(csv, "%d,%s,%d,%.6f,%.2f\n", \
                P_val, label, r, times[r], bw_oop(P_val, n_base, times[r])); \
        sum += times[r]; \
    } \
    double avg = sum / RUNS; \
    printf("  %-14s %8.4f ms  %7.1f GB/s\n", label, avg, bw_oop(P_val, n_base, avg)); \
} while (0)

template<int P>
static void bench_aos(int64_t n) {
    verify_aos<P>(n < 1<<16 ? n : 1<<16);
    size_t bytes = n * sizeof(AoS<P>);
    auto *in  = (AoS<P> *)numa_alloc(bytes); init_aos<P>(in, n);
    auto *out = (AoS<P> *)numa_alloc(bytes); init_aos<P>(out, n);
    CPU_BENCH(P, n, "AoS", (kern_aos<P>(in, out, n)));
    numa_free(in, bytes); numa_free(out, bytes);
}

template<int P>
static void bench_soa(int64_t n) {
    verify_soa<P>(n < 1<<16 ? n : 1<<16);
    size_t bytes = n * sizeof(double);
    size_t pad   = (size_t)4 * P * 64;     /* offset budget: 4P arrays total */
    double *re_in[P], *im_in[P], *re_out[P], *im_out[P];
    void   *raw[4 * P];                    /* raw mmap pointers for munmap */

    for (int p = 0; p < P; p++) {
        int k = 4 * p;
        raw[k+0] = numa_alloc(bytes + pad);
        re_in[p] = (double *)((char *)raw[k+0] + (size_t)(k+0) * 64);
        init_and_bind(re_in[p], n);

        raw[k+1] = numa_alloc(bytes + pad);
        im_in[p] = (double *)((char *)raw[k+1] + (size_t)(k+1) * 64);
        init_and_bind(im_in[p], n);

        raw[k+2] = numa_alloc(bytes + pad);
        re_out[p] = (double *)((char *)raw[k+2] + (size_t)(k+2) * 64);
        init_and_bind(re_out[p], n);

        raw[k+3] = numa_alloc(bytes + pad);
        im_out[p] = (double *)((char *)raw[k+3] + (size_t)(k+3) * 64);
        init_and_bind(im_out[p], n);
    }

    CPU_BENCH(P, n, "SoA", (kern_soa<P>(re_in, im_in, re_out, im_out, n)));

    for (int p = 0; p < P; p++) {
        int k = 4 * p;
        numa_free(raw[k+0], bytes + pad);
        numa_free(raw[k+1], bytes + pad);
        numa_free(raw[k+2], bytes + pad);
        numa_free(raw[k+3], bytes + pad);
    }
}

template<int P, int VL>
static void bench_aosoa(int64_t n, const char *label) {
    int64_t nverif = n < 1<<16 ? n : 1<<16;
    nverif = (nverif / VL) * VL;           /* align to VL for the block loop */
    if (nverif > 0) verify_aosoa<P, VL>(nverif, label);
    int64_t nblks = n / VL;
    size_t bytes = nblks * sizeof(AoSoA<P,VL>);
    auto *in  = (AoSoA<P,VL> *)numa_alloc(bytes); init_aosoa<P,VL>(in, n);
    auto *out = (AoSoA<P,VL> *)numa_alloc(bytes); init_aosoa<P,VL>(out, n);
    CPU_BENCH(P, n, label, (kern_aosoa<P,VL>(in, out, n)));
    numa_free(in, bytes); numa_free(out, bytes);
}

template<int P>
static void run_all() {
    int64_t n = (TOTAL_DOUBLES / (2 * P) / MAX_VL) * MAX_VL;
    printf("\n── P=%d complex pairs  (%d SoA streams)  N_base=%lld  "
           "total=%.1f GB/side ──\n",
           P, 4*P, (long long)n, 2.0 * P * n * 8 / 1e9);

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
    const char *csv_path = (argc > 1) ? argv[1] : "results_cpu_oop.csv";
    flush_init();

    csv = fopen(csv_path, "w");
    fprintf(csv, "P,layout,run,ms,gbps\n");

    printf("conjugate OOP (CPU): TOTAL_DOUBLES=%lldM  runs=%d  threads=%d\n",
           (long long)(TOTAL_DOUBLES >> 20), (int)RUNS, omp_get_max_threads());
    printf("BW = 4·P·N_base·8  (read 2P + write 2P fields)\n");

    {
        int nodes[8] = {};
        #pragma omp parallel
        { int nd = get_numa_node(); if (nd >= 0 && nd < 8) {
            #pragma omp atomic
            nodes[nd]++;
        }}
        printf("  thread spread:");
        for (int i = 0; i < 8; i++) if (nodes[i]) printf(" N%d=%d", i, nodes[i]);
        printf("\n");
    }

    run_all< 3>();   //  6 re/im arrays →  12 SoA streams (in) + 12 (out)
    run_all< 6>();   // 12 re/im arrays →  24 streams
    run_all< 9>();   // 18 re/im arrays →  36 streams
    run_all<12>();   // 24 re/im arrays →  48 streams
    run_all<15>();   // 30 re/im arrays →  60 streams
    run_all<18>();   // 36 re/im arrays →  72 streams
    run_all<21>();   // 42 re/im arrays →  84 streams

    fclose(csv);
    flush_free();
    printf("\nwrote results_cpu_oop.csv\n");
}