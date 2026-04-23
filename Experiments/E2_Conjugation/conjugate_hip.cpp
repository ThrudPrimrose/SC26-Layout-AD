/*
 * HIP entry point for the shared GPU source. The CUDA/HIP macro shim
 * in common/gpu_compat.cuh (pulled in transitively via the .cu file)
 * dispatches cuda*/gpu* calls to hip* under hipcc. No duplicated
 * kernel code needed here.
 */
#include "conjugate.cu"
