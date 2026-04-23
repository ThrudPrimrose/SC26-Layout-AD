/*
 * bench_gpu_hip.cpp -- HIP-compiler entry point for the NumaStream GPU
 * sweep. The real implementation lives in bench_gpu.cu; that file
 * carries a CUDA/HIP macro shim at the top (detects __HIP_PLATFORM_AMD__),
 * so the same source compiles under both nvcc and hipcc.
 *
 * This stub just `#include`s bench_gpu.cu so the build systems that
 * bucket *.cpp files to hipcc and *.cu files to nvcc both find an entry
 * point. run_beverin.sh compiles this file with `hipcc -x hip`.
 */
#include "bench_gpu.cu"
