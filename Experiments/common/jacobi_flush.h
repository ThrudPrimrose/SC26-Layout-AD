/*
 * jacobi_flush.h -- canonical CPU cache-flush kernel.
 *
 * A single implementation shared by every microbenchmark in this repo
 * so that all experiments flush caches with identical work between
 * timed runs. The work is a 2-D 5-point Jacobi sweep at N=8192 run for
 * FLUSH_STEPS iterations with buffer swaps; the combined working set
 * is 2 * 8192^2 * sizeof(double) = 1 GiB, which exceeds the LLC on
 * every target platform (Zen4: 32 MiB/CCX, Grace: 117 MiB/socket).
 *
 * The header is header-only and self-contained: it uses plain mmap()
 * for the backing allocation (MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE
 * so pages are NUMA-placed at first touch) and OpenMP for parallelism.
 *
 * Allocation guarantees:
 *   - The two N*N double buffers are allocated exactly once per process
 *     lifetime (function-level static with mmap on first use).
 *   - Buffer first-touch runs inside an `omp parallel for` so pages
 *     land across NUMA nodes using the first-touch policy.
 *   - Every subsequent flush reuses the same buffers; no allocation
 *     occurs per flush call.
 *
 * All hot-path pointers carry `__restrict__` to license aliasing-free
 * vectorization.
 *
 * API:
 *   flush_jacobi()        -- runs FLUSH_STEPS sweeps in a fresh parallel
 *                            region. Caller must be in a serial context.
 *                            Triggers the one-time init on first call.
 *   flush_jacobi_inner()  -- runs FLUSH_STEPS sweeps inside an existing
 *                            parallel region (uses `#pragma omp for`).
 *                            flush_jacobi_init() MUST have been called
 *                            from a serial context beforehand.
 *   flush_jacobi_init()   -- explicit serial-context init (idempotent).
 */
#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "numa_util.h"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace jacobi_flush_detail {

constexpr int N = 8192;
constexpr int STEPS = 3;

inline double* _alloc_buf() {
  /* Unfaulted, huge-page-advised. First-touch happens in the init
   * below, inside an `omp parallel for`, spreading pages across NUMA
   * nodes. Exactly one allocation per buffer per process lifetime. */
  return numa_alloc_unfaulted<double>((size_t)N * (size_t)N);
}

/* Function-level statics: each called exactly once across the whole
 * process. C++11 guarantees the init is thread-safe, so even if two
 * OpenMP threads raced into _buf_a() simultaneously only one would
 * actually call _alloc_buf(). In practice callers should trigger init
 * from a serial context via flush_jacobi_init() so no race ever occurs. */
inline double* _buf_a() {
  static double* const p = _alloc_buf();
  return p;
}
inline double* _buf_b() {
  static double* const p = _alloc_buf();
  return p;
}

inline bool& _initialized() { static bool v = false; return v; }

inline void _step(const double* __restrict__ A,
                  double* __restrict__ B) {
  #pragma omp for schedule(static)
  for (int i = 1; i < N - 1; ++i) {
    for (int j = 1; j < N - 1; ++j) {
      B[i * N + j] = 0.25 * (A[(i - 1) * N + j] + A[(i + 1) * N + j]
                           + A[i * N + (j - 1)] + A[i * N + (j + 1)]);
    }
  }
}

}  // namespace jacobi_flush_detail

/* One-time, serial-context initialization. Must be called from outside
 * any `omp parallel` region. Safe (idempotent) to call repeatedly. */
inline void flush_jacobi_init() {
  using namespace jacobi_flush_detail;
  if (_initialized()) return;
  double* __restrict__ a = _buf_a();
  double* __restrict__ b = _buf_b();
  /* Parallel first-touch spreads pages across NUMA nodes. */
  #pragma omp parallel for schedule(static)
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      size_t k = (size_t)i * N + j;
      /* Deterministic, well-scaled values so the stencil stays finite. */
      double v = (double)((k * 2654435761ULL) & 0xFFFFF) / 1048576.0;
      a[k] = v;
      b[k] = v;
    }
  }
  _initialized() = true;
}

inline void flush_jacobi() {
  using namespace jacobi_flush_detail;
  flush_jacobi_init();
  double* __restrict__ a = _buf_a();
  double* __restrict__ b = _buf_b();
  #pragma omp parallel
  {
    for (int s = 0; s < STEPS; ++s) {
      if ((s & 1) == 0) _step(a, b);
      else              _step(b, a);
    }
  }
}

inline void flush_jacobi_inner() {
  using namespace jacobi_flush_detail;
  assert(_initialized() &&
         "flush_jacobi_inner() requires a prior serial flush_jacobi_init()");
  double* __restrict__ a = _buf_a();
  double* __restrict__ b = _buf_b();
  for (int s = 0; s < STEPS; ++s) {
    if ((s & 1) == 0) _step(a, b);
    else              _step(b, a);
  }
}
