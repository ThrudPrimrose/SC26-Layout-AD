/*
 * prng.h -- canonical, deterministic PRNG for every bench in this repo.
 *
 * Rationale
 * ---------
 * Earlier revisions used a mix of `srand(0)`, `srand(time(NULL))`,
 * `std::mt19937`, and raw `rand()` calls across E1--E6. That made
 * results non-reproducible across runs (and across machines, since
 * `rand()` is libc-specific). This header centralizes random-number
 * generation so every experiment uses:
 *
 *   - the same PRNG (xorshift64* from Marsaglia, 2003),
 *   - the same canonical seed (42),
 *   - the same helper functions for uniform / bounded draws.
 *
 * Every random draw anywhere in the artifact is reproducible byte-for-
 * byte given the same seed; the seed is always 42 unless a caller
 * derives a per-object seed via `splitmix64(42 ^ tag)` for array-level
 * independence.
 *
 * The PRNG is deliberately XOR-based (pure bit-shift / XOR) so the
 * implementation is trivially auditable, has no hidden table lookups,
 * and emits identical bytes on CPU and GPU.
 */
#pragma once

#include <cstddef>
#include <cstdint>

/* ------------------------------------------------------------------ */
/*  Canonical seed                                                     */
/* ------------------------------------------------------------------ */
#ifndef SC26_SEED
#define SC26_SEED 42u
#endif

/* ------------------------------------------------------------------ */
/*  splitmix64 - integer mixing function (Sebastiano Vigna)            */
/*  Used to derive independent 64-bit seeds from a scalar index.       */
/* ------------------------------------------------------------------ */
static inline uint64_t splitmix64(uint64_t x) {
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

/* ------------------------------------------------------------------ */
/*  xorshift64* (Marsaglia, 2003) - pure XOR/shift PRNG                */
/* ------------------------------------------------------------------ */
static inline uint64_t xor64_next(uint64_t& state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

struct Xor64Rng {
  uint64_t state;

  /* The zero-state is a fixed point for xorshift64*, so we always fold
   * through splitmix64() first. Seed defaults to SC26_SEED. */
  explicit Xor64Rng(uint64_t seed = SC26_SEED) : state(splitmix64(seed)) {
    if (state == 0) state = SC26_SEED;
  }

  /* Raw 64-bit draw. */
  uint64_t next() { return xor64_next(state); }

  /* Uniform double in [0, 1). 53 bits of mantissa. */
  double uniform01() {
    return (double)(next() >> 11) / (double)(1ULL << 53);
  }

  /* Uniform double in [lo, hi). */
  double uniform(double lo, double hi) { return lo + (hi - lo) * uniform01(); }

  /* Uniform int in [0, n). */
  int bounded(int n) { return (int)(next() % (uint64_t)n); }

  /* Bernoulli(p). */
  bool bernoulli(double p) { return uniform01() < p; }
};

/* ------------------------------------------------------------------ */
/*  Deterministic buffer fill helpers                                  */
/*                                                                     */
/*  Every byte written depends only on the element index and the       */
/*  caller-supplied tag. Thread-safe without synchronization because   */
/*  each index is mixed independently via splitmix64.                  */
/* ------------------------------------------------------------------ */

/* Fill `n` doubles in [-5, 5) derived from `seed`. */
static inline void prng_fill_double(double* arr, size_t n, uint64_t seed) {
  for (size_t i = 0; i < n; ++i) {
    uint64_t h = splitmix64(seed * 2654435761ULL + i);
    arr[i] = (double)(int64_t)(h & 0xFFFFF) / 100000.0 - 5.0;
  }
}

/* Fill `n` doubles uniformly in [0, 1). */
static inline void prng_fill_unit(double* arr, size_t n, uint64_t seed) {
  for (size_t i = 0; i < n; ++i) {
    uint64_t h = splitmix64(seed * 2654435761ULL + i);
    arr[i] = (double)(h >> 11) / (double)(1ULL << 53);
  }
}

/* Fill `n` ints uniformly in [lo, hi). */
static inline void prng_fill_int(int* arr, size_t n, int lo, int hi, uint64_t seed) {
  int range = hi - lo;
  for (size_t i = 0; i < n; ++i) {
    uint64_t h = splitmix64(seed * 2654435761ULL + i);
    arr[i] = lo + (int)(h % (uint64_t)range);
  }
}

/* Fill `n` ints as a 0/1 mask with P(true) = p_true. */
static inline void prng_fill_mask(int* arr, size_t n, double p_true, uint64_t seed) {
  for (size_t i = 0; i < n; ++i) {
    uint64_t h = splitmix64(seed * 2654435761ULL + i);
    double u = (double)(h & 0xFFFFFF) / 16777216.0;
    arr[i] = (u < p_true) ? 1 : 0;
  }
}
