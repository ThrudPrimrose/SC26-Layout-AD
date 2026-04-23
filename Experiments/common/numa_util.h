/*
 * numa_util.h -- shared NUMA-aware allocation and first-touch helpers.
 *
 * A single header, used across every bench in this repo, that exposes
 * the allocation primitives each experiment was previously reimplement-
 * ing locally. All functions are `static inline` (header-only) so there
 * is no separate translation unit to link, and no ODR issues if the
 * header is included from multiple .cpp files.
 *
 * Policy decisions (applied globally):
 *   1. Backing memory is mmap()-reserved unfaulted. Pages are NOT
 *      committed until first touch, which assigns them to the running
 *      thread's NUMA node (default Linux policy MPOL_DEFAULT).
 *   2. `MADV_HUGEPAGE` is advised on every allocation so that large
 *      buffers use 2 MiB transparent huge pages when the kernel can
 *      back them. This is the uniform policy for all experiments.
 *   3. `first_touch_zero` / `first_touch_copy` run inside an OpenMP
 *      parallel for with `schedule(static)` so distinct threads zero
 *      distinct page-aligned stripes, triggering NUMA first-touch.
 *   4. `bind_and_touch` explicitly partitions the buffer across NUMA
 *      nodes via `mbind(MPOL_BIND, ...)`. Use when the downstream
 *      iteration pattern is fixed and not captured by the first-touch
 *      schedule.
 *
 * Public API:
 *   Byte-based (drop-in for prior local `numa_alloc`/`numa_free`):
 *     void*  numa_alloc_bytes(size_t bytes);
 *     void   numa_free_bytes(void* p, size_t bytes);
 *     void*  numa_alloc(size_t bytes);            // alias of _bytes
 *     void   numa_free(void* p, size_t bytes);    // alias of _bytes
 *
 *   Count-based, typed:
 *     T*     numa_alloc<T>(size_t count);
 *     T*     numa_alloc_unfaulted<T>(size_t count);  // alias
 *     void   numa_dealloc<T>(T* p, size_t count);
 *
 *   First-touch helpers:
 *     void   first_touch_zero<T>(T* arr, size_t count);
 *     void   first_touch_copy<T>(T* dst, const T* src, size_t count);
 *
 *   Explicit per-page binding (mbind):
 *     void   bind_and_touch(void* base, size_t total_bytes);
 *
 *   Introspection:
 *     int    numa_num_nodes();
 *     size_t numa_page_size();
 */
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#if __has_include(<numaif.h>)
  #include <numaif.h>
  #define NUMA_UTIL_HAS_NUMAIF 1
#else
  #define NUMA_UTIL_HAS_NUMAIF 0
  #ifndef MPOL_BIND
    #define MPOL_BIND 2
  #endif
  #ifndef MPOL_INTERLEAVE
    #define MPOL_INTERLEAVE 3
  #endif
  #ifndef MPOL_MF_MOVE
    #define MPOL_MF_MOVE 2
  #endif
  static inline long mbind(void*, unsigned long, int, const unsigned long*, unsigned long, unsigned) {
    return 0;  /* stub: no-op when numaif.h is unavailable */
  }
  static inline long move_pages(int, unsigned long, void**, const int*, int*, int) {
    return -1;
  }
#endif

/* Linux syscall wrapper for move_pages(). We query per-page status by
 * passing nodes=NULL; the kernel then fills status[i] with the node
 * currently hosting pages[i], or a negative error code. */
#ifndef NUMA_UTIL_HAS_MOVE_PAGES
#define NUMA_UTIL_HAS_MOVE_PAGES NUMA_UTIL_HAS_NUMAIF
#endif

/* ------------------------------------------------------------------ */
/*  Introspection                                                      */
/* ------------------------------------------------------------------ */
static inline size_t numa_page_size() {
  static const size_t p = (size_t)sysconf(_SC_PAGESIZE);
  return p;
}

/* Current CPU's NUMA node for the calling thread. Uses the getcpu()
 * Linux syscall. Returns 0 if the syscall is unavailable. Useful for
 * per-thread NUMA-affinity logging inside parallel regions. */
static inline int get_numa_node() {
#ifdef __linux__
  unsigned cpu = 0, node = 0;
  if (syscall(__NR_getcpu, &cpu, &node, nullptr) == 0)
    return (int)node;
#endif
  return 0;
}

static inline int numa_num_nodes() {
  static int cached = 0;
  if (cached) return cached;
  int n = 0;
  for (int i = 0; i < 128; ++i) {
    char path[128];
    std::snprintf(path, sizeof(path), "/sys/devices/system/node/node%d", i);
    if (access(path, F_OK) == 0) n = i + 1;
    else if (n) break;
  }
  cached = (n > 0) ? n : 1;
  return cached;
}

/* ------------------------------------------------------------------ */
/*  Byte-based allocation                                              */
/* ------------------------------------------------------------------ */
static inline void* numa_alloc_bytes(size_t bytes) {
  if (bytes == 0) return nullptr;
  void* p = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (p == MAP_FAILED) {
    std::perror("numa_alloc_bytes/mmap");
    std::abort();
  }
  /* Global policy: huge-page-backed whenever the kernel can provide. */
  madvise(p, bytes, MADV_HUGEPAGE);
  return p;
}

static inline void numa_free_bytes(void* p, size_t bytes) {
  if (p) munmap(p, bytes);
}

static inline void* numa_alloc(size_t bytes)              { return numa_alloc_bytes(bytes); }
static inline void  numa_free(void* p, size_t bytes)      { numa_free_bytes(p, bytes); }

/* ------------------------------------------------------------------ */
/*  Templated, count-based allocation                                  */
/* ------------------------------------------------------------------ */
template <typename T>
static inline T* numa_alloc(size_t count) {
  return reinterpret_cast<T*>(numa_alloc_bytes(count * sizeof(T)));
}

template <typename T>
static inline T* numa_alloc_unfaulted(size_t count) {
  /* Alias kept so callers written against the previous convention
   * (E4, loopnest_*) need no edits. */
  return numa_alloc<T>(count);
}

template <typename T>
static inline void numa_dealloc(T* p, size_t count) {
  numa_free_bytes(reinterpret_cast<void*>(p), count * sizeof(T));
}

/* ------------------------------------------------------------------ */
/*  First-touch helpers                                                */
/* ------------------------------------------------------------------ */
template <typename T>
static inline void first_touch_zero(T* __restrict__ arr, size_t count) {
  #pragma omp parallel for schedule(static)
  for (size_t i = 0; i < count; ++i) arr[i] = T{};
}

template <typename T>
static inline void first_touch_copy(T* __restrict__ dst,
                                    const T* __restrict__ src,
                                    size_t count) {
  #pragma omp parallel for schedule(static)
  for (size_t i = 0; i < count; ++i) dst[i] = src[i];
}

/* ------------------------------------------------------------------ */
/*  Explicit per-chunk binding                                         */
/* ------------------------------------------------------------------ */
/* Partition [base, base+total_bytes) into D contiguous chunks aligned
 * to page boundaries and mbind each chunk to its own NUMA node. Then
 * first-touch every page to realize the binding.
 *
 * MPOL_MF_MOVE is passed so that pages that were already faulted on a
 * different node (e.g. from an earlier setup variant) are migrated to
 * the requested node rather than left in place. This makes the call
 * idempotent: the resulting distribution is always 0,1,2,...,D-1 for
 * the four contiguous stripes, regardless of the buffer's prior state.
 *
 * Safe to call even when libnuma / numaif.h is unavailable (mbind
 * becomes a no-op; distribution reverts to the kernel's default). */
static inline void bind_and_touch(void* base, size_t total_bytes) {
  if (!base || !total_bytes) return;
  const size_t ps = numa_page_size();
  const int D = numa_num_nodes();
  const size_t total_pages = (total_bytes + ps - 1) / ps;
  const size_t pages_per_node = (total_pages + D - 1) / D;

  for (int d = 0; d < D; ++d) {
    size_t plo = (size_t)d * pages_per_node * ps;
    size_t phi = (size_t)(d + 1) * pages_per_node * ps;
    if (phi > total_pages * ps) phi = total_pages * ps;
    if (phi <= plo) continue;
    unsigned long mask = 1UL << d;
    mbind(reinterpret_cast<char*>(base) + plo, phi - plo, MPOL_BIND,
          &mask, 64, MPOL_MF_MOVE);
  }

  /* First-touch one byte per page. The parallel_for distributes page
   * writes across threads; combined with the mbind above each page is
   * now both bound and resident. */
  #pragma omp parallel for schedule(static)
  for (size_t i = 0; i < total_bytes; i += ps) {
    volatile char* p = reinterpret_cast<volatile char*>(base) + i;
    *p = 0;
  }
}

/* Apply MPOL_INTERLEAVE across all D NUMA nodes with MPOL_MF_MOVE so any
 * previously-placed pages are re-distributed round-robin. Then touch
 * every page to realize the new placement. */
static inline void interleave_and_touch(void* base, size_t total_bytes) {
  if (!base || !total_bytes) return;
  const size_t ps = numa_page_size();
  const int D = numa_num_nodes();
  unsigned long mask = 0;
  for (int d = 0; d < D; ++d) mask |= (1UL << d);
  const size_t total_pages = (total_bytes + ps - 1) / ps;
  mbind(base, total_pages * ps, MPOL_INTERLEAVE, &mask, 64, MPOL_MF_MOVE);
  #pragma omp parallel for schedule(static)
  for (size_t i = 0; i < total_bytes; i += ps) {
    volatile char* p = reinterpret_cast<volatile char*>(base) + i;
    *p = 0;
  }
}

/* ------------------------------------------------------------------ */
/*  Page-placement introspection                                       */
/* ------------------------------------------------------------------ */
/* Query the actual NUMA node hosting each page of [base, base+bytes).
 * Returns a vector of length ceil(bytes / page_size); each entry is
 * the kernel-reported node id, or a negative value if move_pages()
 * could not resolve that page (e.g. not yet faulted in). */
static inline std::vector<int> numa_page_nodes(const void* base, size_t bytes) {
  const size_t ps = numa_page_size();
  const size_t npages = (bytes + ps - 1) / ps;
  std::vector<int> status(npages, -1);
#if NUMA_UTIL_HAS_MOVE_PAGES
  std::vector<void*> addrs(npages);
  for (size_t i = 0; i < npages; ++i) {
    addrs[i] = reinterpret_cast<void*>(
                 reinterpret_cast<uintptr_t>(base) + i * ps);
  }
  /* pid=0 means the calling process; nodes=NULL means "query status"  */
  /* rather than "migrate". flags=0 is required for the query form.    */
  long rc = move_pages(0, (unsigned long)npages,
                       addrs.data(), nullptr, status.data(), 0);
  (void)rc;
#else
  (void)base;
#endif
  return status;
}

/* Human-readable summary of numa_page_nodes(). Prints one line per NUMA
 * node with both absolute page count and percentage of the buffer.
 * Useful to verify that a bind_and_touch / interleave_and_touch actually
 * did what we asked for. */
static inline void numa_report_distribution(const void* base, size_t bytes,
                                            const char* label = nullptr) {
  auto st = numa_page_nodes(base, bytes);
  const size_t ps = numa_page_size();
  const int D = numa_num_nodes();
  std::vector<size_t> hist((size_t)(D + 1), 0);   /* last slot = unplaced */
  for (int n : st) {
    if (n >= 0 && n < D) hist[(size_t)n]++;
    else                 hist[(size_t)D]++;
  }
  const size_t total = st.size();
  const double mib = (double)total * (double)ps / (1024.0 * 1024.0);
  std::printf("[numa] %s  %zu pages  %.1f MiB (ps=%zu KiB)\n",
              label ? label : "distribution", total, mib, ps / 1024);
  for (int d = 0; d < D; ++d) {
    double pct = total ? 100.0 * (double)hist[(size_t)d] / (double)total : 0.0;
    std::printf("  node %d : %8zu pages (%5.1f%%)\n", d, hist[(size_t)d], pct);
  }
  if (hist[(size_t)D])
    std::printf("  unplaced: %8zu pages\n", hist[(size_t)D]);
}
