/*
 * bench_stream_gpu.cu -- 2D GPU STREAM sweep (scale-add RMW) on an
 * N x N fp32 matrix. Kernel:  A[y,x] = s * A[y,x] + B[y,x]
 * Traffic: 2 loads + 1 store per fp32 = 12 bytes per element.
 *
 * Modelled after E1_MatrixAdd/bench_gpu.cu: templated kernels with
 * compile-time (BX, BY, TX, TY), __launch_bounds__ hint, per-iteration
 * CSV records, NREP=100/NWARMUP=5, coalesced (strided) per-thread tile.
 *
 * Sweeps the cross-product of:
 *   - thread-block shapes  (BX, BY)   with BX*BY <= 1024 and >= 32
 *   - per-thread work      (TX, TY)   elements each thread owns
 * Block covers (BX*TX) x (BY*TY) elements. Indexing is strided so
 * adjacent threads along x stay on adjacent memory (coalesced).
 *
 * Same source compiles for CUDA (nvcc) and HIP (hipcc -x hip).
 *
 * Every kernel config is run twice -- once with a canonical 8192^2
 * Jacobi cache flush between reps (flush=yes), once without (flush=no).
 * The "no flush" pass matches the legacy common/bench_stream_gpu.cu
 * methodology (warm HBM / L2 state after warmup); the "yes flush" pass
 * forces cold cache between every timed rep. Headline statistic is the
 * arithmetic *mean* across reps (not median), because for NUMA we care
 * about the achievable peak rather than a robust central estimate.
 *
 * Output CSV (one row per repetition):
 *   kernel,BX,BY,TX,TY,N,rep,time_ms,bw_gbs,checksum,status,flush
 *
 * Usage: bench_stream_gpu <csv_file> [N1d=16384]
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

#include "../common/gpu_compat.cuh"
#include "../common/jacobi_flush_gpu.cuh"

#ifndef N1D_DIM
#define N1D_DIM 16384
#endif

static constexpr int Nd = N1D_DIM;

#define NWARMUP 5
#define NREP    100

/* ================================================================
 *  Kernel: 2D scale-add with (BX, BY) block and (TX, TY) per-thread tile.
 *  Strided indexing keeps adjacent threads coalesced even when TX > 1.
 * ================================================================ */
template<int BX, int BY, int TX, int TY>
__global__ void __launch_bounds__(BX * BY)
kernel_scale_add_2d(float *__restrict__ A,
                    const float *__restrict__ B,
                    float s)
{
    constexpr int TILE_COLS = BX * TX;
    constexpr int TILE_ROWS = BY * TY;

    const size_t bx0 = (size_t)blockIdx.x * TILE_COLS;
    const size_t by0 = (size_t)blockIdx.y * TILE_ROWS;

    #pragma unroll
    for (int dy = 0; dy < TY; dy++) {
        size_t y = by0 + threadIdx.y + (size_t)dy * BY;
        if (y >= (size_t)Nd) break;
        #pragma unroll
        for (int dx = 0; dx < TX; dx++) {
            size_t x = bx0 + threadIdx.x + (size_t)dx * BX;
            if (x >= (size_t)Nd) break;
            size_t i = y * Nd + x;
            A[i] = s * A[i] + B[i];
        }
    }
}

/* init: A = 1.0, B = 2.0 so one RMW gives A[i] = 1.0001 + 2.0 = 3.0001 */
__global__ void init_kernel(float *__restrict__ A, float *__restrict__ B) {
    size_t idx = (size_t)blockIdx.x * blockDim.x + threadIdx.x;
    size_t total = (size_t)Nd * Nd;
    size_t stride = (size_t)gridDim.x * blockDim.x;
    for (; idx < total; idx += stride) {
        A[idx] = 1.0f;
        B[idx] = 2.0f;
    }
}

/* ================================================================
 *  Registration: templated launcher wrapped in std::function
 * ================================================================ */
using LaunchFn = std::function<void(float *, const float *, float)>;

struct KernelConfig {
    std::string name;
    int BX, BY, TX, TY;
    LaunchFn    launch;
};

static std::vector<KernelConfig> g_configs;

template<int BX, int BY, int TX, int TY>
static void reg_config() {
    constexpr int TILE_COLS = BX * TX;
    constexpr int TILE_ROWS = BY * TY;
    dim3 blk(BX, BY);
    dim3 grd((Nd + TILE_COLS - 1) / TILE_COLS,
             (Nd + TILE_ROWS - 1) / TILE_ROWS);
    if (!gpu_launch_valid(grd, blk)) return;
    char name[96];
    snprintf(name, sizeof(name),
             "scale_add_2d BX=%-3d BY=%-3d TX=%-2d TY=%-2d  tile=%dx%d",
             BX, BY, TX, TY, TILE_ROWS, TILE_COLS);
    g_configs.push_back({name, BX, BY, TX, TY,
        [blk, grd](float *A, const float *B, float s) {
            kernel_scale_add_2d<BX, BY, TX, TY><<<grd, blk>>>(A, B, s);
        }});
}

static void register_configs() {
    /* Block shapes chosen so BX*BY is in [32, 1024] and BX >= warp / 2
     * so consecutive threads stay contiguous in x (coalesced). */

    /* --- 1D blocks (BY = 1): vary TX, TY --- */
    reg_config<  32, 1, 1, 1>();
    reg_config<  32, 1, 2, 1>();
    reg_config<  32, 1, 4, 1>();
    reg_config<  32, 1, 1, 2>();
    reg_config<  32, 1, 2, 2>();
    reg_config<  32, 1, 4, 4>();
    reg_config< 128, 1, 1, 1>();
    reg_config< 128, 1, 1, 2>();
    reg_config< 128, 1, 2, 2>();
    reg_config< 128, 1, 4, 4>();
    reg_config< 256, 1, 1, 1>();
    reg_config< 256, 1, 1, 2>();
    reg_config< 256, 1, 1, 4>();

    /* --- square-ish blocks --- */
    reg_config<  16, 16, 1, 1>();
    reg_config<  16, 16, 2, 1>();
    reg_config<  16, 16, 1, 2>();
    reg_config<  16, 16, 2, 2>();
    reg_config<  16, 16, 4, 1>();
    reg_config<  16, 16, 1, 4>();
    reg_config<  16, 16, 4, 4>();
    reg_config<  32, 32, 1, 1>();
    reg_config<  32, 32, 2, 1>();
    reg_config<  32, 32, 1, 2>();
    reg_config<  32, 32, 2, 2>();

    /* --- 2D rectangular: favour wider BX for coalescing --- */
    reg_config<  32,  4, 1, 1>();
    reg_config<  32,  4, 2, 1>();
    reg_config<  32,  4, 4, 1>();
    reg_config<  32,  4, 1, 4>();
    reg_config<  32,  4, 4, 4>();
    reg_config<  32,  8, 1, 1>();
    reg_config<  32,  8, 2, 1>();
    reg_config<  32,  8, 1, 2>();
    reg_config<  32,  8, 2, 2>();
    reg_config<  32,  8, 4, 4>();
    reg_config<  32,  8, 8, 1>();
    reg_config<  32,  8, 1, 8>();
    reg_config<  64,  4, 1, 1>();
    reg_config<  64,  4, 2, 2>();
    reg_config<  64,  4, 4, 4>();
    reg_config<  64,  8, 1, 1>();
    reg_config<  64,  8, 2, 2>();
    reg_config<  64, 16, 1, 1>();
    reg_config< 128,  2, 1, 1>();
    reg_config< 128,  2, 2, 2>();
    reg_config< 128,  4, 1, 1>();
    reg_config< 128,  4, 2, 2>();
    reg_config< 128,  8, 1, 1>();
}

/* ================================================================
 *  Per-iteration record + benchmark harness
 * ================================================================ */
struct IterRecord {
    std::string kernel;
    int BX, BY, TX, TY;
    int rep;
    float time_ms;
    double bw_gbs;
    double checksum;
    bool ok;
    bool flush;
};

static std::vector<IterRecord> g_records;

static void run_bench(const KernelConfig &cfg, float *dA, float *dB,
                      float *hA_scratch, double ref_cs_after_one,
                      bool use_flush)
{
    const size_t total = (size_t)Nd * Nd;
    const double data_bytes = 3.0 * (double)total * sizeof(float);

    /* Reset A so the correctness check is deterministic. */
    const int init_blk = 256;
    const int init_grd = 65536;
    init_kernel<<<init_grd, init_blk>>>(dA, dB);
    GPU_CHECK(gpuDeviceSynchronize());

    /* Correctness: run once, read back, reduce. */
    cfg.launch(dA, dB, 1.0001f);
    GPU_CHECK(gpuDeviceSynchronize());
    GPU_CHECK(gpuMemcpy(hA_scratch, dA, total * sizeof(float), gpuMemcpyDeviceToHost));
    double cs = 0.0;
    for (size_t k = 0; k < total; k++) cs += (double)hA_scratch[k];
    bool ok = std::fabs(cs - ref_cs_after_one) <= 1e-3 * std::fabs(ref_cs_after_one);
    if (!ok) {
        fprintf(stderr, "  [%s flush=%s] CHECKSUM MISMATCH: got %.6e exp %.6e\n",
                cfg.name.c_str(), use_flush ? "yes" : "no", cs, ref_cs_after_one);
    }

    /* Warmups -- we don't reset A because the RMW is measurement-stable
     * for bandwidth (values grow but traffic per rep is identical). When
     * flushing, flush between warmup reps too so the first timed rep
     * starts from the same cold state as every subsequent one. */
    for (int w = 0; w < NWARMUP; w++) {
        if (use_flush) flush_jacobi_gpu();
        cfg.launch(dA, dB, 1.0001f);
    }
    GPU_CHECK(gpuDeviceSynchronize());

    gpuEvent_t t0, t1;
    GPU_CHECK(gpuEventCreate(&t0));
    GPU_CHECK(gpuEventCreate(&t1));

    float times[NREP];
    for (int r = 0; r < NREP; r++) {
        /* Flush is enqueued on the default stream before the event record,
         * so t0 fires only after all flush kernels finish -- no flush time
         * leaks into the measured interval. */
        if (use_flush) flush_jacobi_gpu();
        GPU_CHECK(gpuEventRecord(t0));
        cfg.launch(dA, dB, 1.0001f);
        GPU_CHECK(gpuEventRecord(t1));
        GPU_CHECK(gpuEventSynchronize(t1));
        GPU_CHECK(gpuEventElapsedTime(&times[r], t0, t1));

        double bw = data_bytes / ((double)times[r] * 1e-3) / 1e9;
        g_records.push_back({cfg.name, cfg.BX, cfg.BY, cfg.TX, cfg.TY,
                             r, times[r], bw, cs, ok, use_flush});
    }
    GPU_CHECK(gpuEventDestroy(t0));
    GPU_CHECK(gpuEventDestroy(t1));

    double sum_ms = 0.0, sum_bw = 0.0;
    for (int r = 0; r < NREP; r++) {
        sum_ms += (double)times[r];
        sum_bw += data_bytes / ((double)times[r] * 1e-3) / 1e9;
    }
    double mean_ms = sum_ms / (double)NREP;
    double mean_bw = sum_bw / (double)NREP;
    printf("  %-55s  flush=%-3s  %8.3f ms  %8.1f GB/s  %s\n",
           cfg.name.c_str(), use_flush ? "yes" : "no",
           mean_ms, mean_bw, ok ? "OK" : "FAIL");
}

/* ================================================================ */
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <csv_file>\n", argv[0]);
        return 1;
    }
    const char *csv_path = argv[1];

    gpuDeviceProp prop;
    GPU_CHECK(gpuGetDeviceProperties(&prop, 0));
    printf("NUMA STREAM peak GPU (E0) on %s\n", prop.name);
    printf("  N1d=%d  (%.2f GiB/buf)  reps=%d  warmup=%d  backend=%s\n\n",
           Nd, ((double)Nd * Nd * sizeof(float)) / (double)(1UL << 30),
           NREP, NWARMUP, GPU_BACKEND_NAME);

    const size_t total = (size_t)Nd * Nd;
    const size_t bytes = total * sizeof(float);

    float *dA = nullptr, *dB = nullptr;
    GPU_CHECK(gpuMalloc(&dA, bytes));
    GPU_CHECK(gpuMalloc(&dB, bytes));
    float *hA = (float *)malloc(bytes);
    if (!hA) { fprintf(stderr, "host malloc failed\n"); return 1; }

    /* Ref checksum: one RMW on A=1.0, B=2.0 gives A[i] = 1.0001 * 1.0 + 2.0 */
    const double ref_cs = 3.0001 * (double)total;

    register_configs();

    for (bool use_flush : { true, false }) {
        printf("\n######## flush=%s ########\n", use_flush ? "yes" : "no");
        printf("%-55s  %-9s  %10s  %10s  %s\n",
               "Kernel", "flush", "mean ms", "BW", "ok");
        printf("%s\n", std::string(96, '-').c_str());
        for (const auto &cfg : g_configs)
            run_bench(cfg, dA, dB, hA, ref_cs, use_flush);
    }

    /* Per-iteration CSV for violin plots. */
    FILE *fp = fopen(csv_path, "w");
    if (!fp) { perror(csv_path); return 1; }
    fprintf(fp, "kernel,BX,BY,TX,TY,N,rep,time_ms,bw_gbs,checksum,status,flush\n");
    for (const auto &r : g_records)
        fprintf(fp, "\"%s\",%d,%d,%d,%d,%zu,%d,%.6f,%.3f,%.6e,%s,%s\n",
                r.kernel.c_str(), r.BX, r.BY, r.TX, r.TY, total,
                r.rep, (double)r.time_ms, r.bw_gbs, r.checksum,
                r.ok ? "PASS" : "FAIL",
                r.flush ? "yes" : "no");
    fclose(fp);
    printf("\nWrote %zu records to %s\n", g_records.size(), csv_path);

    GPU_CHECK(gpuFree(dA));
    GPU_CHECK(gpuFree(dB));
    free(hA);
    return 0;
}
