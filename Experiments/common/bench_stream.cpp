/*
 * bench_stream.cpp -- NUMA-aware CPU STREAM peak (scale-add RMW).
 *
 * Kernel:  A[i] = s * A[i] + B[i]
 * Traffic: 2 loads + 1 store per fp32 element (12 bytes per element).
 *
 * Buffers are 2D square (N x N, fp32) and use:
 *   - mmap(MAP_ANONYMOUS | MAP_NORESERVE) so pages are unfaulted
 *   - madvise(MADV_HUGEPAGE) so the kernel backs with transparent hugepages
 *   - first-touch block initialization under the active OMP binding so pages
 *     land on the NUMA node of the thread that owns the row range
 *
 * Output CSV (single row, overwrites file each run):
 *   device,bw_gbs,bw_tbs,N,bytes_per_iter,reps,threads
 *
 * Usage: bench_stream [out.csv=stream_peak_cpu.csv] [N=16384] [reps=50]
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <omp.h>

static float *alloc_huge(size_t bytes) {
    void *p = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); exit(1); }
    madvise(p, bytes, MADV_HUGEPAGE);
    return (float *)p;
}

static void first_touch(float *__restrict__ a, size_t N, float val) {
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int nth = omp_get_num_threads();
        size_t lo = (size_t)N * tid / nth;
        size_t hi = (size_t)N * (tid + 1) / nth;
        for (size_t i = lo; i < hi; i++) a[i] = val;
    }
}

static double run_scale_add(float *__restrict__ A, const float *__restrict__ B, size_t N, int reps) {
    const float s = 1.0001f;
    /* warmup */
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int nth = omp_get_num_threads();
        size_t lo = (size_t)N * tid / nth;
        size_t hi = (size_t)N * (tid + 1) / nth;
        for (size_t i = lo; i < hi; i++) A[i] = s * A[i] + B[i];
    }
    double best = 1e30;
    for (int r = 0; r < reps; r++) {
        double t0 = 0, t1 = 0;
        #pragma omp parallel
        {
            #pragma omp barrier
            #pragma omp master
            t0 = omp_get_wtime();
            #pragma omp barrier

            int tid = omp_get_thread_num();
            int nth = omp_get_num_threads();
            size_t lo = (size_t)N * tid / nth;
            size_t hi = (size_t)N * (tid + 1) / nth;
            for (size_t i = lo; i < hi; i++) A[i] = s * A[i] + B[i];

            #pragma omp barrier
            #pragma omp master
            t1 = omp_get_wtime();
        }
        double dt = t1 - t0;
        if (dt < best) best = dt;
    }
    /* 2 loads + 1 store = 3 * sizeof(float) bytes per element */
    return 3.0 * (double)N * sizeof(float) / best;  /* bytes / s */
}

int main(int argc, char **argv) {
    const char *out   = (argc > 1) ? argv[1] : "stream_peak_cpu.csv";
    size_t       N1d  = (argc > 2) ? (size_t)atoll(argv[2]) : 16384;  /* square edge */
    int          reps = (argc > 3) ? atoi(argv[3]) : 50;

    size_t N     = N1d * N1d;
    size_t bytes = N * sizeof(float);
    int    P     = omp_get_max_threads();

    fprintf(stderr, "[bench_stream cpu] N=%zux%zu (%.2f GiB per buffer), threads=%d, reps=%d\n",
            N1d, N1d, bytes / (double)(1UL << 30), P, reps);

    float *A = alloc_huge(bytes);
    float *B = alloc_huge(bytes);
    first_touch(A, N, 1.0f);
    first_touch(B, N, 2.0f);

    double bw_bps = run_scale_add(A, B, N, reps);
    double bw_gbs = bw_bps * 1e-9;
    double bw_tbs = bw_bps * 1e-12;
    fprintf(stderr, "[bench_stream cpu] peak = %.2f GB/s (%.3f TB/s)\n", bw_gbs, bw_tbs);

    FILE *f = fopen(out, "w");
    if (!f) { perror("fopen"); return 1; }
    fprintf(f, "device,bw_gbs,bw_tbs,N,bytes_per_iter,reps,threads\n");
    fprintf(f, "cpu,%.4f,%.6f,%zu,%zu,%d,%d\n",
            bw_gbs, bw_tbs, N, (size_t)(3 * bytes), reps, P);
    fclose(f);

    munmap(A, bytes);
    munmap(B, bytes);
    return 0;
}
