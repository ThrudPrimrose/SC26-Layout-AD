/*
 * jacobi_flush_gpu.cuh -- canonical GPU cache-flush kernel.
 *
 * The GPU counterpart to common/jacobi_flush.h. Both sides run the
 * same algorithm on the same canonical size so timings taken on CPU
 * and GPU benches are comparable with respect to how much "background
 * work" sits between two timed iterations:
 *
 *   - N = 8192  (two 8192 * 8192 * sizeof(double) = 1 GiB buffers,
 *                more than 10x the L2 of any current HPC GPU and much
 *                larger than MI300A's ~256 MiB Infinity Cache).
 *   - STEPS = 3 swept 5-point Jacobi sweeps with buffer swap.
 *   - Allocated exactly once at first use; subsequent calls reuse the
 *     same device buffers.
 *
 * The source is shared between CUDA and HIP via the same macro shim
 * that every other GPU file in this repo uses. When compiling a .cu
 * under nvcc the HIP_PLATFORM macros are absent, so the CUDA branch is
 * picked; when compiling a .cpp under hipcc with -x hip the HIP branch
 * is picked.
 *
 * Public API (all inline, header-only):
 *   void flush_jacobi_gpu_init();    -- one-time allocation + init;
 *                                       safe to call repeatedly.
 *   void flush_jacobi_gpu();         -- run STEPS Jacobi sweeps on the
 *                                       persistent buffers; to be
 *                                       called between timed reps.
 *   void flush_jacobi_gpu_destroy(); -- optional, frees the device
 *                                       buffers (normally left until
 *                                       process exit).
 */
#pragma once

#include <cstdio>
#include <cstdlib>
#include <utility>

#include "gpu_compat.cuh"        /* cuda/hip unification lives here */
#define JACOBI_FLUSH_GPU_CHECK GPU_CHECK

namespace jacobi_flush_gpu_detail {

constexpr int N = 8192;
constexpr int STEPS = 3;
constexpr int BX = 16;
constexpr int BY = 16;

/* `inline` on `__global__` triggers [-Wcuda-compat] in clang-hip and is
 * ignored anyway. Each benchmark links exactly one TU that includes
 * this header (the bench_*.cu per experiment), so a bare __global__
 * has no ODR / multiple-definition risk. */
__global__ void _step(const double *__restrict__ A,
                      double *__restrict__ B) {
  int i = blockIdx.y * BY + threadIdx.y;
  int j = blockIdx.x * BX + threadIdx.x;
  if (i > 0 && i < N - 1 && j > 0 && j < N - 1) {
    B[i * N + j] = 0.25 * (A[(i - 1) * N + j] + A[(i + 1) * N + j]
                         + A[i * N + (j - 1)] + A[i * N + (j + 1)]);
  }
}

__global__ void _init(double *__restrict__ buf) {
  size_t idx = (size_t)blockIdx.x * blockDim.x + threadIdx.x;
  size_t stride = (size_t)gridDim.x * blockDim.x;
  size_t total = (size_t)N * (size_t)N;
  for (size_t k = idx; k < total; k += stride) {
    /* Same deterministic pattern as common/jacobi_flush.h. */
    double v = (double)((k * 2654435761ULL) & 0xFFFFF) / 1048576.0;
    buf[k] = v;
  }
}

inline double*& _buf_a() { static double *p = nullptr; return p; }
inline double*& _buf_b() { static double *p = nullptr; return p; }
inline bool& _initialized() { static bool v = false; return v; }

}  // namespace jacobi_flush_gpu_detail

inline void flush_jacobi_gpu_init() {
  using namespace jacobi_flush_gpu_detail;
  if (_initialized()) return;
  size_t bytes = (size_t)N * (size_t)N * sizeof(double);
  JACOBI_FLUSH_GPU_CHECK(gpuMalloc(reinterpret_cast<void **>(&_buf_a()), bytes));
  JACOBI_FLUSH_GPU_CHECK(gpuMalloc(reinterpret_cast<void **>(&_buf_b()), bytes));
  dim3 block(256, 1, 1), grid(4096, 1, 1);
  _init<<<grid, block>>>(_buf_a());
  _init<<<grid, block>>>(_buf_b());
  JACOBI_FLUSH_GPU_CHECK(gpuDeviceSynchronize());
  _initialized() = true;
}

inline void flush_jacobi_gpu() {
  using namespace jacobi_flush_gpu_detail;
  flush_jacobi_gpu_init();
  double *a = _buf_a();
  double *b = _buf_b();
  dim3 block(BX, BY, 1);
  dim3 grid((N + BX - 1) / BX, (N + BY - 1) / BY, 1);
  for (int s = 0; s < STEPS; ++s) {
    if ((s & 1) == 0) _step<<<grid, block>>>(a, b);
    else              _step<<<grid, block>>>(b, a);
  }
  /* no explicit synchronize here; the caller's event timing / next
   * kernel launch will serialize naturally on the same stream. */
}

inline void flush_jacobi_gpu_destroy() {
  using namespace jacobi_flush_gpu_detail;
  if (!_initialized()) return;
  JACOBI_FLUSH_GPU_CHECK(gpuFree(_buf_a()));
  JACOBI_FLUSH_GPU_CHECK(gpuFree(_buf_b()));
  _buf_a() = nullptr;
  _buf_b() = nullptr;
  _initialized() = false;
}
