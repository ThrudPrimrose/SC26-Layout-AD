/*
 * gpu_compat.cuh -- single canonical CUDA/HIP compatibility shim.
 *
 * All GPU sources in the repo (`bench_gpu.cu`, `bench_gpu_hip.cpp`,
 * `jacobi_flush_gpu.cuh`, ...) should `#include` this header instead of
 * defining their own local macro shim. That keeps the CUDA <-> HIP
 * dispatch in exactly one place, so adding, renaming, or versioning a
 * runtime symbol only has to happen here.
 *
 * Expected build recipe:
 *   nvcc  -c bench_gpu.cu        -> CUDA path (default branch below)
 *   hipcc -x hip bench_gpu.cu    -> HIP path (hipcc predefines
 *                                   __HIP_PLATFORM_AMD__)
 *
 * For the HIP side we also support the companion ".cpp" stub pattern
 * used elsewhere in the repo: `bench_gpu_hip.cpp` is usually just
 * `#include "bench_gpu.cu"`, so hipcc has a .cpp file to bucket while
 * still compiling exactly the same source.
 *
 * Policy: every GPU runtime call goes through a `gpu*` macro defined
 * here; callers should not use `cudaMalloc` / `hipMalloc` directly.
 * Sources that need a vendor-specific library (cuTENSOR, hipTensor,
 * CUTLASS, rocBLAS) are the only justified exceptions and must live in
 * a separately compiled translation unit (e.g. E3_Transpose has
 * `transpose_cutensor.cu` and `transpose_hiptensor.cpp`).
 */
#pragma once

#include <cstdio>
#include <cstdlib>

/* hipcc -x hip predefines __HIPCC__ / __HIP__; downstream users may also
 * set __HIP_PLATFORM_AMD__ manually. Accept all three so the HIP branch
 * is selected regardless of how the build was invoked. */
#if defined(__HIPCC__) || defined(__HIP__) \
    || defined(__HIP_PLATFORM_AMD__) || defined(HIP_PLATFORM_AMD)
  #include <hip/hip_runtime.h>

  /* ---- types ---- */
  using gpuError_t          = hipError_t;
  using gpuEvent_t          = hipEvent_t;
  using gpuStream_t         = hipStream_t;
  using gpuDeviceProp       = hipDeviceProp_t;

  /* ---- memory management ---- */
  #define gpuMalloc                hipMalloc
  #define gpuMallocManaged         hipMallocManaged
  #define gpuFree                  hipFree
  #define gpuMemcpy                hipMemcpy
  #define gpuMemcpyAsync           hipMemcpyAsync
  #define gpuMemset                hipMemset
  #define gpuMemsetAsync           hipMemsetAsync

  /* ---- copy directions ---- */
  #define gpuMemcpyHostToDevice    hipMemcpyHostToDevice
  #define gpuMemcpyDeviceToHost    hipMemcpyDeviceToHost
  #define gpuMemcpyDeviceToDevice  hipMemcpyDeviceToDevice
  #define gpuMemcpyHostToHost      hipMemcpyHostToHost

  /* ---- synchronization ---- */
  #define gpuDeviceSynchronize     hipDeviceSynchronize
  #define gpuStreamSynchronize     hipStreamSynchronize

  /* ---- events ---- */
  #define gpuEventCreate           hipEventCreate
  #define gpuEventDestroy          hipEventDestroy
  #define gpuEventRecord           hipEventRecord
  #define gpuEventSynchronize      hipEventSynchronize
  #define gpuEventElapsedTime      hipEventElapsedTime

  /* ---- streams ---- */
  #define gpuStreamCreate          hipStreamCreate
  #define gpuStreamDestroy         hipStreamDestroy

  /* ---- device / error ---- */
  #define gpuGetDevice             hipGetDevice
  #define gpuSetDevice             hipSetDevice
  #define gpuGetDeviceCount        hipGetDeviceCount
  #define gpuGetDeviceProperties   hipGetDeviceProperties
  #define gpuGetLastError          hipGetLastError
  #define gpuGetErrorString        hipGetErrorString

  static constexpr gpuError_t gpuSuccess = hipSuccess;

  /* ---- cuda* aliases so legacy bench code written against CUDA
   *      compiles under hipcc unchanged. nvcc already has the cuda*
   *      symbols natively, so these defines only exist on the HIP side. */
  #define cudaMalloc                 hipMalloc
  #define cudaMallocManaged          hipMallocManaged
  #define cudaFree                   hipFree
  #define cudaMemcpy                 hipMemcpy
  #define cudaMemcpyAsync            hipMemcpyAsync
  #define cudaMemset                 hipMemset
  #define cudaMemsetAsync            hipMemsetAsync
  #define cudaMemcpyHostToDevice     hipMemcpyHostToDevice
  #define cudaMemcpyDeviceToHost     hipMemcpyDeviceToHost
  #define cudaMemcpyDeviceToDevice   hipMemcpyDeviceToDevice
  #define cudaMemcpyHostToHost       hipMemcpyHostToHost
  #define cudaDeviceSynchronize      hipDeviceSynchronize
  #define cudaStreamSynchronize      hipStreamSynchronize
  #define cudaEventCreate            hipEventCreate
  #define cudaEventDestroy           hipEventDestroy
  #define cudaEventRecord            hipEventRecord
  #define cudaEventSynchronize       hipEventSynchronize
  #define cudaEventElapsedTime       hipEventElapsedTime
  #define cudaStreamCreate           hipStreamCreate
  #define cudaStreamDestroy          hipStreamDestroy
  #define cudaGetDevice              hipGetDevice
  #define cudaSetDevice              hipSetDevice
  #define cudaGetDeviceCount         hipGetDeviceCount
  #define cudaGetDeviceProperties    hipGetDeviceProperties
  #define cudaGetLastError           hipGetLastError
  #define cudaGetErrorString         hipGetErrorString
  using cudaError_t                = hipError_t;
  using cudaEvent_t                = hipEvent_t;
  using cudaStream_t               = hipStream_t;
  using cudaDeviceProp             = hipDeviceProp_t;
  static constexpr cudaError_t cudaSuccess = hipSuccess;

  /* ---- backend identification (for conditional `#ifdef`-free code) ---- */
  #define GPU_BACKEND_HIP  1
  #define GPU_BACKEND_CUDA 0
  #define GPU_BACKEND_NAME "HIP"
#else
  #include <cuda_runtime.h>

  /* ---- types ---- */
  using gpuError_t          = cudaError_t;
  using gpuEvent_t          = cudaEvent_t;
  using gpuStream_t         = cudaStream_t;
  using gpuDeviceProp       = cudaDeviceProp;

  /* ---- memory management ---- */
  #define gpuMalloc                cudaMalloc
  #define gpuMallocManaged         cudaMallocManaged
  #define gpuFree                  cudaFree
  #define gpuMemcpy                cudaMemcpy
  #define gpuMemcpyAsync           cudaMemcpyAsync
  #define gpuMemset                cudaMemset
  #define gpuMemsetAsync           cudaMemsetAsync

  /* ---- copy directions ---- */
  #define gpuMemcpyHostToDevice    cudaMemcpyHostToDevice
  #define gpuMemcpyDeviceToHost    cudaMemcpyDeviceToHost
  #define gpuMemcpyDeviceToDevice  cudaMemcpyDeviceToDevice
  #define gpuMemcpyHostToHost      cudaMemcpyHostToHost

  /* ---- synchronization ---- */
  #define gpuDeviceSynchronize     cudaDeviceSynchronize
  #define gpuStreamSynchronize     cudaStreamSynchronize

  /* ---- events ---- */
  #define gpuEventCreate           cudaEventCreate
  #define gpuEventDestroy          cudaEventDestroy
  #define gpuEventRecord           cudaEventRecord
  #define gpuEventSynchronize      cudaEventSynchronize
  #define gpuEventElapsedTime      cudaEventElapsedTime

  /* ---- streams ---- */
  #define gpuStreamCreate          cudaStreamCreate
  #define gpuStreamDestroy         cudaStreamDestroy

  /* ---- device / error ---- */
  #define gpuGetDevice             cudaGetDevice
  #define gpuSetDevice             cudaSetDevice
  #define gpuGetDeviceCount        cudaGetDeviceCount
  #define gpuGetDeviceProperties   cudaGetDeviceProperties
  #define gpuGetLastError          cudaGetLastError
  #define gpuGetErrorString        cudaGetErrorString

  static constexpr gpuError_t gpuSuccess = cudaSuccess;

  #define GPU_BACKEND_HIP  0
  #define GPU_BACKEND_CUDA 1
  #define GPU_BACKEND_NAME "CUDA"
#endif

/* ------------------------------------------------------------------ */
/*  Canonical error check.                                             */
/*                                                                     */
/*  Every GPU runtime call should be wrapped:                          */
/*     GPU_CHECK(gpuMalloc(&p, bytes));                                */
/*                                                                     */
/*  On a non-success return it prints the error string with file:LINE  */
/*  context and aborts so failures can never be silently ignored.      */
/* ------------------------------------------------------------------ */
#define GPU_CHECK(expr) do {                                              \
    gpuError_t _e = (expr);                                               \
    if (_e != gpuSuccess) {                                               \
      std::fprintf(stderr,                                                \
                   "[GPU " GPU_BACKEND_NAME " error] %s:%d: %s\n",        \
                   __FILE__, __LINE__, gpuGetErrorString(_e));            \
      std::abort();                                                       \
    }                                                                      \
  } while (0)

/* Legacy alias -- existing call sites written against CUDA can keep      */
/* using CUDA_CHECK(...) unchanged. Guarded so a file that still carries  */
/* its own local definition does not trip a redefinition warning.        */
#ifndef CUDA_CHECK
#define CUDA_CHECK(expr) GPU_CHECK(expr)
#endif

/* ------------------------------------------------------------------ */
/*  Launch-configuration sanity check.                                 */
/*                                                                     */
/*  Matches the shared Hopper (SM_90) / MI300A limits used across the  */
/*  artifact; adequate for every kernel in this repo. Returns true iff */
/*  the given (grid, block) would be accepted by the driver.           */
/*                                                                     */
/*    blockDim.x * y * z <= 1024                                       */
/*    blockDim.z         <= 64                                         */
/*    gridDim.y          <= 65535                                      */
/*    gridDim.z          <= 65535                                      */
/*    gridDim.x          <= 2^31 - 1  (de facto unbounded for us)      */
/*                                                                     */
/*  Kept as a free function so runners can precheck and skip invalid   */
/*  sweeps cleanly, without relying on a post-launch cudaGetLastError. */
/* ------------------------------------------------------------------ */
static inline bool gpu_launch_valid(unsigned gx, unsigned gy, unsigned gz,
                                    unsigned bx, unsigned by, unsigned bz) {
  if ((unsigned long long)bx * by * bz > 1024ULL) return false;
  if (bz > 64) return false;
  if (gy > 65535u) return false;
  if (gz > 65535u) return false;
  if (gx > 2147483647u) return false;
  return true;
}
static inline bool gpu_launch_valid(dim3 grid, dim3 block) {
  return gpu_launch_valid(grid.x, grid.y, grid.z, block.x, block.y, block.z);
}
