/*
 * HIP entry point for the shared bench_stream_gpu source.
 * hipcc is happy compiling .cpp; the macro shim inside the included file
 * dispatches to hip* calls when __HIP_PLATFORM_AMD__ is set by the build.
 */
#include "bench_stream_gpu.cu"
