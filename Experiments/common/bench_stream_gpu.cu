/*
 * bench_stream_gpu.cu -- GPU STREAM peak (scale-add RMW) on a huge square
 * fp32 matrix. Kernel:  A[i] = s * A[i] + B[i]
 * Traffic: 2 loads + 1 store per fp32 = 12 bytes per element.
 *
 * Same source compiles for CUDA (nvcc) and HIP (hipcc) via the shim below.
 *
 * Output CSV (single row, overwrites file each run):
 *   device,bw_gbs,bw_tbs,N,bytes_per_iter,reps
 *
 * Usage: bench_stream_gpu [out.csv=stream_peak_gpu.csv] [N=16384] [reps=50]
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>

#if defined(__HIP_PLATFORM_AMD__) || defined(HIP_PLATFORM_AMD)
  #include <hip/hip_runtime.h>
  #define gpuMalloc          hipMalloc
  #define gpuFree            hipFree
  #define gpuMemset          hipMemset
  #define gpuEventCreate     hipEventCreate
  #define gpuEventRecord     hipEventRecord
  #define gpuEventSynchronize hipEventSynchronize
  #define gpuEventElapsedTime hipEventElapsedTime
  #define gpuEventDestroy    hipEventDestroy
  #define gpuDeviceSynchronize hipDeviceSynchronize
  #define gpuEvent_t         hipEvent_t
  #define gpuSuccess         hipSuccess
  #define gpuGetLastError    hipGetLastError
  #define gpuGetErrorString  hipGetErrorString
#else
  #include <cuda_runtime.h>
  #define gpuMalloc          cudaMalloc
  #define gpuFree            cudaFree
  #define gpuMemset          cudaMemset
  #define gpuEventCreate     cudaEventCreate
  #define gpuEventRecord     cudaEventRecord
  #define gpuEventSynchronize cudaEventSynchronize
  #define gpuEventElapsedTime cudaEventElapsedTime
  #define gpuEventDestroy    cudaEventDestroy
  #define gpuDeviceSynchronize cudaDeviceSynchronize
  #define gpuEvent_t         cudaEvent_t
  #define gpuSuccess         cudaSuccess
  #define gpuGetLastError    cudaGetLastError
  #define gpuGetErrorString  cudaGetErrorString
#endif

#define CK(expr) do { auto _e = (expr); if (_e != gpuSuccess) { \
    fprintf(stderr, "GPU error: %s\n", gpuGetErrorString(_e)); exit(1); } } while(0)

__global__ void scale_add(float *A, const float *B, size_t N, float s) {
    size_t i = (size_t)blockIdx.x * blockDim.x + threadIdx.x;
    size_t stride = (size_t)gridDim.x * blockDim.x;
    for (; i < N; i += stride) A[i] = s * A[i] + B[i];
}

__global__ void fill(float *A, size_t N, float v) {
    size_t i = (size_t)blockIdx.x * blockDim.x + threadIdx.x;
    size_t stride = (size_t)gridDim.x * blockDim.x;
    for (; i < N; i += stride) A[i] = v;
}

int main(int argc, char **argv) {
    const char *out   = (argc > 1) ? argv[1] : "stream_peak_gpu.csv";
    size_t       N1d  = (argc > 2) ? (size_t)atoll(argv[2]) : 16384;
    int          reps = (argc > 3) ? atoi(argv[3]) : 50;

    size_t N     = N1d * N1d;
    size_t bytes = N * sizeof(float);

    fprintf(stderr, "[bench_stream gpu] N=%zux%zu (%.2f GiB per buffer), reps=%d\n",
            N1d, N1d, bytes / (double)(1UL << 30), reps);

    float *A = nullptr, *B = nullptr;
    CK(gpuMalloc((void **)&A, bytes));
    CK(gpuMalloc((void **)&B, bytes));

    int block = 256;
    int grid  = 65536;  /* grid-stride; saturates any modern GPU */

    fill<<<grid, block>>>(A, N, 1.0f);
    fill<<<grid, block>>>(B, N, 2.0f);
    CK(gpuDeviceSynchronize());

    /* warmup */
    scale_add<<<grid, block>>>(A, B, N, 1.0001f);
    CK(gpuDeviceSynchronize());

    gpuEvent_t e0, e1;
    CK(gpuEventCreate(&e0));
    CK(gpuEventCreate(&e1));

    float best_ms = 1e30f;
    for (int r = 0; r < reps; r++) {
        CK(gpuEventRecord(e0));
        scale_add<<<grid, block>>>(A, B, N, 1.0001f);
        CK(gpuEventRecord(e1));
        CK(gpuEventSynchronize(e1));
        float ms = 0;
        CK(gpuEventElapsedTime(&ms, e0, e1));
        if (ms < best_ms) best_ms = ms;
    }
    CK(gpuEventDestroy(e0));
    CK(gpuEventDestroy(e1));

    double bw_bps = 3.0 * (double)N * sizeof(float) / (best_ms * 1e-3);
    double bw_gbs = bw_bps * 1e-9;
    double bw_tbs = bw_bps * 1e-12;
    fprintf(stderr, "[bench_stream gpu] peak = %.2f GB/s (%.3f TB/s)\n", bw_gbs, bw_tbs);

    FILE *f = fopen(out, "w");
    if (!f) { perror("fopen"); return 1; }
    fprintf(f, "device,bw_gbs,bw_tbs,N,bytes_per_iter,reps\n");
    fprintf(f, "gpu,%.4f,%.6f,%zu,%zu,%d\n",
            bw_gbs, bw_tbs, N, (size_t)(3 * bytes), reps);
    fclose(f);

    CK(gpuFree(A));
    CK(gpuFree(B));
    return 0;
}
