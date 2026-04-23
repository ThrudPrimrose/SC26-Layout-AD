/*
 * HIP entry point for the shared bench_gpu source. The CUDA/HIP macro
 * shim in common/gpu_compat.cuh (pulled in transitively via
 * bench_gpu.cu) dispatches gpu* calls to hip* when __HIPCC__ is set by
 * the hipcc -x hip build. No duplicated kernel code needed here.
 */
#include "bench_gpu.cu"
