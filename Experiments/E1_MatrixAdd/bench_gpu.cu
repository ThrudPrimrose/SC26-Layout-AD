/*
 * Layout-conflict benchmark (GPU) — parameterized sweep
 * ======================================================
 * C[i,j] += A[i,j] + B[i,j]
 *
 * Layouts:  A row-major, B col-major, C row-major
 *
 * Kernels:
 *   direct       — no shared memory; threadIdx.x → j (row-coalesced for A,C)
 *   direct_T     — no shared memory; threadIdx.x → i (col-coalesced for B)
 *   tiled+smem   — cooperative load/store with per-layout coalescing
 *   all_rowmajor — control (B also row-major → peak BW)
 *
 * Parameterized by:
 *   BX, BY   — thread block dimensions
 *   TX, TY   — elements per thread (thread computes TX × TY rectangle)
 *   Tile dims derived: TILE_COLS = BX*TX,  TILE_ROWS = BY*TY
 *
 * Compile: nvcc -O3 -Xcompiler=-fno-vect-cost-model -arch=sm_90 -std=c++17 -o bench_gpu bench_gpu.cpp
 * Run:     ./bench_gpu [csv_file]
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include "../common/gpu_compat.cuh"    /* single CUDA/HIP shim */

/* ---- dimensions ---- */
#ifndef M_DIM
#define M_DIM 16384
#endif
#ifndef N_DIM
#define N_DIM 16384
#endif

static constexpr int Md = M_DIM;
static constexpr int Nd = N_DIM;

#define NWARMUP  5
#define NREP     100

/* GPU_CHECK is provided by ../common/gpu_compat.cuh. */


/* ================================================================
 *  COALESCED LOAD / STORE PRIMITIVES
 * ================================================================
 *
 * Design principle: consecutive threads (consecutive linear tid)
 * must hit consecutive addresses in global memory.
 *
 *   ROW_MAJOR A[i*N + j]:  consecutive j → consecutive addrs
 *     → linearize tile as k = li * TILE_C + lj
 *       so consecutive k → consecutive lj → consecutive j  ✓
 *
 *   COL_MAJOR B[j*M + i]:  consecutive i → consecutive addrs
 *     → linearize tile as k = lj * TILE_R + li
 *       so consecutive k → consecutive li → consecutive i  ✓
 *
 * Shared memory is always row-major with +1 padding to avoid
 * bank conflicts: smem[li * (TILE_C+1) + lj].
 * ================================================================ */

enum Layout { ROW_MAJOR = 0, COL_MAJOR = 1 };

/*
 * coop_load<TILE_R, TILE_C, L>
 *   Cooperatively loads a TILE_R × TILE_C tile from global memory
 *   into shared memory.  Access pattern is coalesced for layout L.
 *   Shared memory stride is TILE_C + 1 (bank-conflict padding).
 */
template<int TILE_R, int TILE_C, Layout L>
__device__ __forceinline__
void coop_load(double* __restrict__ smem,
               const double* __restrict__ gmem,
               int bi, int bj,              /* tile origin in global coords */
               int tid, int nthreads)        /* linear thread id, block size */
{
    constexpr int STRIDE = TILE_C + 1;      /* padded smem row stride */
    constexpr int TOTAL  = TILE_R * TILE_C;

    for (int k = tid; k < TOTAL; k += nthreads) {
        int li, lj;
        if constexpr (L == ROW_MAJOR) {
            /* consecutive k → consecutive lj → consecutive j → coalesced */
            li = k / TILE_C;
            lj = k % TILE_C;
        } else {
            /* consecutive k → consecutive li → consecutive i → coalesced */
            li = k % TILE_R;
            lj = k / TILE_R;
        }

        int gi = bi + li, gj = bj + lj;
        double val = 0.0;
        if (gi < Md && gj < Nd) {
            if constexpr (L == ROW_MAJOR)
                val = gmem[gi * Nd + gj];      /* A[i*N + j] */
            else
                val = gmem[gj * Md + gi];      /* B[j*M + i] */
        }
        smem[li * STRIDE + lj] = val;          /* always row-major in smem */
    }
}

/*
 * coop_store<TILE_R, TILE_C, L>
 *   Cooperatively stores a TILE_R × TILE_C tile from shared memory
 *   to global memory.  Access pattern is coalesced for layout L.
 */
template<int TILE_R, int TILE_C, Layout L>
__device__ __forceinline__
void coop_store(const double* __restrict__ smem,
                double* __restrict__ gmem,
                int bi, int bj,
                int tid, int nthreads)
{
    constexpr int STRIDE = TILE_C + 1;
    constexpr int TOTAL  = TILE_R * TILE_C;

    for (int k = tid; k < TOTAL; k += nthreads) {
        int li, lj;
        if constexpr (L == ROW_MAJOR) {
            li = k / TILE_C;
            lj = k % TILE_C;
        } else {
            li = k % TILE_R;
            lj = k / TILE_R;
        }

        int gi = bi + li, gj = bj + lj;
        if (gi < Md && gj < Nd) {
            if constexpr (L == ROW_MAJOR)
                gmem[gi * Nd + gj] += smem[li * STRIDE + lj];
            else
                gmem[gj * Md + gi] += smem[li * STRIDE + lj];
        }
    }
}


/* ================================================================
 *  KERNEL: direct global (threadIdx.x → j, row-coalesced for A,C)
 *  A,C coalesced ✓   B uncoalesced ✗
 * ================================================================ */
template<int BX, int BY, int TX, int TY>
__global__ void __launch_bounds__(BX * BY)
kernel_direct(const double* __restrict__ A,
              const double* __restrict__ B,
              double*       __restrict__ C)
{
    constexpr int TILE_COLS = BX * TX;
    constexpr int TILE_ROWS = BY * TY;

    int base_j = blockIdx.x * TILE_COLS + threadIdx.x;
    int base_i = blockIdx.y * TILE_ROWS + threadIdx.y;

    #pragma unroll
    for (int dy = 0; dy < TY; dy++) {
        int i = base_i + dy * BY;
        #pragma unroll
        for (int dx = 0; dx < TX; dx++) {
            int j = base_j + dx * BX;
            if (i < Md && j < Nd)
                C[i * Nd + j] += A[i * Nd + j] + B[j * Md + i];
        }
    }
}

/* ================================================================
 *  KERNEL: direct transposed (threadIdx.x → i, col-coalesced for B)
 *  B coalesced ✓   A,C uncoalesced ✗
 * ================================================================ */
template<int BX, int BY, int TX, int TY>
__global__ void __launch_bounds__(BX * BY)
kernel_direct_T(const double* __restrict__ A,
                const double* __restrict__ B,
                double*       __restrict__ C)
{
    constexpr int TILE_ROWS = BX * TX;    /* note: BX maps to i */
    constexpr int TILE_COLS = BY * TY;

    int base_i = blockIdx.x * TILE_ROWS + threadIdx.x;  /* tidx → i */
    int base_j = blockIdx.y * TILE_COLS + threadIdx.y;

    #pragma unroll
    for (int dx = 0; dx < TX; dx++) {
        int i = base_i + dx * BX;
        #pragma unroll
        for (int dy = 0; dy < TY; dy++) {
            int j = base_j + dy * BY;
            if (i < Md && j < Nd)
                C[i * Nd + j] += A[i * Nd + j] + B[j * Md + i];
        }
    }
}

/* ================================================================
 *  KERNEL: tiled + shared memory (coalesced loads for all layouts)
 *
 *  Uses coop_load<ROW_MAJOR> for A  (tidx sweeps j → coalesced)
 *        coop_load<COL_MAJOR> for B  (tidx sweeps i → coalesced)
 *        coop_store<ROW_MAJOR> for C
 *
 *  Each thread computes a TX × TY sub-tile from shared memory.
 * ================================================================ */
template<int BX, int BY, int TX, int TY>
__global__ void __launch_bounds__(BX * BY)
kernel_tiled(const double* __restrict__ A,
             const double* __restrict__ B,
             double*       __restrict__ C)
{
    constexpr int TILE_COLS = BX * TX;
    constexpr int TILE_ROWS = BY * TY;
    constexpr int STRIDE    = TILE_COLS + 1;       /* padded smem stride */
    constexpr int SMEM_TILE = TILE_ROWS * STRIDE;

    extern __shared__ double smem[];
    double* __restrict__ As = smem;
    double* __restrict__ Bs = smem + SMEM_TILE;
    double* __restrict__ Cs = smem + 2 * SMEM_TILE;

    const int bi  = blockIdx.y * TILE_ROWS;
    const int bj  = blockIdx.x * TILE_COLS;
    const int tid = threadIdx.y * BX + threadIdx.x;
    constexpr int NTHREADS = BX * BY;

    /* ---- cooperative load (coalesced per source layout) ---- */
    coop_load<TILE_ROWS, TILE_COLS, ROW_MAJOR>(As, A, bi, bj, tid, NTHREADS);
    coop_load<TILE_ROWS, TILE_COLS, COL_MAJOR>(Bs, B, bi, bj, tid, NTHREADS);

    __syncthreads();

    /* ---- per-thread compute from shared memory ---- */
    #pragma unroll
    for (int dy = 0; dy < TY; dy++) {
        int li = threadIdx.y + dy * BY;
        #pragma unroll
        for (int dx = 0; dx < TX; dx++) {
            int lj = threadIdx.x + dx * BX;
            int si = li * STRIDE + lj;
            Cs[si] = As[si] + Bs[si];
        }
    }

    __syncthreads();

    /* ---- cooperative store (coalesced for row-major C) ---- */
    coop_store<TILE_ROWS, TILE_COLS, ROW_MAJOR>(Cs, C, bi, bj, tid, NTHREADS);
}

/* ================================================================
 *  KERNEL: all row-major control (B also row-major → peak BW)
 * ================================================================ */
template<int BX, int BY, int TX, int TY>
__global__ void __launch_bounds__(BX * BY)
kernel_control(const double* __restrict__ A,
               const double* __restrict__ B_rm,
               double*       __restrict__ C)
{
    constexpr int TILE_COLS = BX * TX;
    constexpr int TILE_ROWS = BY * TY;

    int base_j = blockIdx.x * TILE_COLS + threadIdx.x;
    int base_i = blockIdx.y * TILE_ROWS + threadIdx.y;

    #pragma unroll
    for (int dy = 0; dy < TY; dy++) {
        int i = base_i + dy * BY;
        #pragma unroll
        for (int dx = 0; dx < TX; dx++) {
            int j = base_j + dx * BX;
            if (i < Md && j < Nd) {
                int idx = i * Nd + j;
                C[idx] += A[idx] + B_rm[idx];
            }
        }
    }
}


/* ================================================================
 *  INIT
 * ================================================================ */
__global__ void init_kernel(double* __restrict__ A,
                            double* __restrict__ B_cm,
                            double* __restrict__ B_rm)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int total = Md * Nd;
    for (; idx < total; idx += blockDim.x * gridDim.x) {
        int i = idx / Nd, j = idx % Nd;
        double va = (double)((i * 17 + j * 31) % 1000) / 100.0;
        double vb = (double)((i * 13 + j * 37) % 1000) / 100.0;
        A[i * Nd + j]     = va;       /* row-major */
        B_cm[j * Md + i]  = vb;       /* col-major */
        B_rm[i * Nd + j]  = vb;       /* row-major copy */
    }
}


/* ================================================================
 *  Configuration & benchmark infrastructure
 * ================================================================ */

/* Unified launch signature: (A, B, C) — B is col-major or row-major
   depending on config. */
using LaunchFn = std::function<void(const double*, const double*, double*)>;

struct KernelConfig {
    std::string name;
    LaunchFn    launch;
    int         tile_rows;
    int         tile_cols;
    size_t      smem_bytes;
    const void* kernel_fn;       /* kernel fn ptr for cudaFuncSetAttribute; */
                                 /* nullptr for kernels with 0 dynamic smem */
    bool        uses_B_rm;       /* true = uses row-major B (control) */
};

static std::vector<KernelConfig> g_configs;

/* ---- registration helpers ---- */
template<int BX, int BY, int TX, int TY>
void reg_direct() {
    constexpr int TR = BY * TY, TC = BX * TX;
    dim3 blk(BX, BY);
    dim3 grd((Nd + TC - 1) / TC, (Md + TR - 1) / TR);
    char name[128];
    snprintf(name, sizeof(name),
             "direct        BX=%-3d BY=%-3d TX=%-2d TY=%-2d  tile=%dx%d",
             BX, BY, TX, TY, TR, TC);
    g_configs.push_back({name,
        [blk, grd](const double* A, const double* B, double* C) {
            kernel_direct<BX, BY, TX, TY><<<grd, blk>>>(A, B, C);
        }, TR, TC, 0, nullptr, false});
}

template<int BX, int BY, int TX, int TY>
void reg_direct_T() {
    constexpr int TR = BX * TX, TC = BY * TY;
    dim3 blk(BX, BY);
    dim3 grd((Md + TR - 1) / TR, (Nd + TC - 1) / TC);
    char name[128];
    snprintf(name, sizeof(name),
             "direct_T      BX=%-3d BY=%-3d TX=%-2d TY=%-2d  tile=%dx%d",
             BX, BY, TX, TY, TR, TC);
    g_configs.push_back({name,
        [blk, grd](const double* A, const double* B, double* C) {
            kernel_direct_T<BX, BY, TX, TY><<<grd, blk>>>(A, B, C);
        }, TR, TC, 0, nullptr, false});
}

template<int BX, int BY, int TX, int TY>
void reg_tiled() {
    constexpr int TR = BY * TY, TC = BX * TX;
    constexpr int STRIDE = TC + 1;
    constexpr size_t smem = 3 * TR * STRIDE * sizeof(double);
    dim3 blk(BX, BY);
    dim3 grd((Nd + TC - 1) / TC, (Md + TR - 1) / TR);
    char name[128];
    snprintf(name, sizeof(name),
             "tiled+smem    BX=%-3d BY=%-3d TX=%-2d TY=%-2d  tile=%dx%d",
             BX, BY, TX, TY, TR, TC);
    const void* kfn = reinterpret_cast<const void*>(
        &kernel_tiled<BX, BY, TX, TY>);
    g_configs.push_back({name,
        [blk, grd, smem](const double* A, const double* B, double* C) {
            kernel_tiled<BX, BY, TX, TY><<<grd, blk, smem>>>(A, B, C);
        }, TR, TC, smem, kfn, false});
}

template<int BX, int BY, int TX, int TY>
void reg_control() {
    constexpr int TR = BY * TY, TC = BX * TX;
    dim3 blk(BX, BY);
    dim3 grd((Nd + TC - 1) / TC, (Md + TR - 1) / TR);
    char name[128];
    snprintf(name, sizeof(name),
             "all_rowmajor  BX=%-3d BY=%-3d TX=%-2d TY=%-2d  (control)",
             BX, BY, TX, TY);
    g_configs.push_back({name,
        [blk, grd](const double* A, const double* B_rm, double* C) {
            kernel_control<BX, BY, TX, TY><<<grd, blk>>>(A, B_rm, C);
        }, TR, TC, 0, nullptr, true});
}

/* ================================================================
 *  Register all configurations to sweep
 * ================================================================ */
void register_configs() {
    /*
     * Notation:  BX×BY thread block,  TX×TY per-thread work
     *            Tile = (BY*TY) rows × (BX*TX) cols
     */

    /* ==== direct: threadIdx.x → j  (A,C coalesced, B strided) ==== */
    reg_direct<  32,  1,  1,  1>();   /* tile  1×32   — minimal */
    reg_direct<  32,  1,  4,  1>();   /* tile  1×128  — wide */
    reg_direct< 128,  1,  1,  1>();   /* tile  1×128  — wide block */
    reg_direct< 256,  1,  1,  1>();   /* tile  1×256  — max 1D */
    reg_direct<  16, 16,  1,  1>();   /* tile 16×16   — square */
    reg_direct<  32,  8,  1,  1>();   /* tile  8×32   — common */
    reg_direct<  32,  8,  2,  2>();   /* tile 16×64   — 2×2 work */
    reg_direct<  32,  4,  4,  4>();   /* tile 16×128  — 4×4 work */
    reg_direct<  32,  8,  4,  4>();   /* tile 32×128  — big tile */
    /* -- extended sweep: fill BX/BY grid --                         */
    reg_direct<  32,  2,  1,  1>();   /* tile  2×32                   */
    reg_direct<  32,  4,  1,  1>();   /* tile  4×32                   */
    reg_direct<  32, 16,  1,  1>();   /* tile 16×32                   */
    reg_direct<  32, 32,  1,  1>();   /* tile 32×32                   */
    reg_direct<  32,  8,  1,  2>();   /* tile 16×32                   */
    reg_direct<  32,  8,  1,  4>();   /* tile 32×32                   */
    reg_direct<  32,  8,  4,  1>();   /* tile  8×128                  */
    /* -- wavefront-aligned for MI300A (BX=64) --                    */
    reg_direct<  64,  1,  1,  1>();   /* tile  1×64   — wavefront     */
    reg_direct<  64,  1,  2,  1>();   /* tile  1×128                  */
    reg_direct<  64,  1,  4,  1>();   /* tile  1×256                  */
    reg_direct<  64,  2,  1,  1>();   /* tile  2×64                   */
    reg_direct<  64,  4,  1,  1>();   /* tile  4×64                   */
    reg_direct<  64,  4,  2,  2>();   /* tile  8×128                  */
    reg_direct<  64,  8,  1,  1>();   /* tile  8×64                   */
    reg_direct<  64,  8,  2,  1>();   /* tile  8×128                  */
    reg_direct<  64,  8,  2,  2>();   /* tile 16×128                  */
    reg_direct<  64, 16,  1,  1>();   /* tile 16×64                   */
    reg_direct<  64,  4,  4,  4>();   /* tile 16×256                  */
    /* -- wider blocks (2+ wavefronts / wider coalesced row) --      */
    reg_direct< 128,  2,  1,  1>();   /* tile  2×128                  */
    reg_direct< 128,  4,  1,  1>();   /* tile  4×128                  */
    reg_direct< 128,  4,  2,  2>();   /* tile  8×256                  */
    reg_direct< 128,  8,  1,  1>();   /* tile  8×128                  */
    reg_direct< 128,  8,  2,  2>();   /* tile 16×256                  */
    reg_direct< 256,  2,  1,  1>();   /* tile  2×256                  */
    reg_direct< 256,  4,  1,  1>();   /* tile  4×256                  */
    reg_direct< 512,  1,  1,  1>();   /* tile  1×512                  */
    reg_direct< 512,  2,  1,  1>();   /* tile  2×512                  */
    reg_direct<1024,  1,  1,  1>();   /* tile  1×1024  (max 1D block) */

    /* ==== direct_T: threadIdx.x → i  (B coalesced, A,C strided) ==== */
    reg_direct_T<  32,  1,  1,  1>();
    reg_direct_T< 128,  1,  1,  1>();
    reg_direct_T<  16, 16,  1,  1>();
    reg_direct_T<  32,  8,  1,  1>();
    reg_direct_T<  32,  8,  2,  2>();
    /* -- extended sweep --                                           */
    reg_direct_T<  32,  4,  1,  1>();
    reg_direct_T<  32, 16,  1,  1>();
    reg_direct_T<  32, 32,  1,  1>();
    reg_direct_T<  16, 16,  2,  2>();
    reg_direct_T<  16, 16,  4,  1>();
    reg_direct_T<  16, 16,  1,  4>();
    /* -- wavefront-aligned (BX=64 here → 64 consecutive i) --       */
    reg_direct_T<  64,  1,  1,  1>();
    reg_direct_T<  64,  4,  1,  1>();
    reg_direct_T<  64,  4,  2,  2>();
    reg_direct_T<  64,  8,  1,  1>();
    reg_direct_T<  64,  8,  2,  2>();
    reg_direct_T<  64, 16,  1,  1>();
    reg_direct_T< 128,  4,  1,  1>();
    reg_direct_T< 128,  8,  1,  1>();
    reg_direct_T<  32,  8,  4,  1>();
    reg_direct_T<  32,  8,  1,  4>();

    /* ==== tiled + shared memory (all coalesced) ====
     * Rule of thumb: smem = 3 * TR * (TC+1) * 8 bytes.
     * MI300A LDS = 64 KiB  →  TR * (TC+1) ≤ 2730 fits without opt-in.
     * H100/H200 via cudaFuncAttributeMaxDynamicSharedMemorySize
     * allow up to ~227 KiB per block; those configs still run there. */
    reg_tiled<  16, 16,  1,  1>();   /* tile 16×16                    */
    reg_tiled<  16, 16,  2,  2>();   /* tile 32×32                    */
    reg_tiled<  32,  1,  1, 32>();   /* tile 32×32   (1D block)       */
    reg_tiled<  32,  8,  1,  1>();   /* tile  8×32                    */
    reg_tiled<  32,  8,  2,  2>();   /* tile 16×64                    */
    reg_tiled<  32,  8,  2,  4>();   /* tile 32×64                    */
    reg_tiled<  32,  4,  2,  8>();   /* tile 32×64   (narrow block)   */
    reg_tiled<  32,  8,  4,  4>();   /* tile 32×128                   */
    reg_tiled<  16, 16,  4,  4>();   /* tile 64×64                    */
    reg_tiled<  32,  8,  4,  8>();   /* tile 64×128                   */
    reg_tiled<  32,  8,  8,  8>();   /* tile 64×256                   */
    reg_tiled<  32,  4,  8, 16>();   /* tile 64×256  (narrow block)   */
    /* -- extended sweep: fill BX/BY grid --                         */
    reg_tiled<  32,  2,  1,  1>();   /* tile  2×32                    */
    reg_tiled<  32,  4,  1,  1>();   /* tile  4×32                    */
    reg_tiled<  32,  4,  2,  2>();   /* tile  8×64                    */
    reg_tiled<  32, 16,  1,  1>();   /* tile 16×32                    */
    reg_tiled<  32, 32,  1,  1>();   /* tile 32×32  (square block)    */
    reg_tiled<  16, 16,  2,  1>();   /* tile 16×32                    */
    reg_tiled<  16, 16,  1,  2>();   /* tile 32×16                    */
    reg_tiled<  32,  8,  1,  2>();   /* tile 16×32                    */
    reg_tiled<  32,  8,  1,  4>();   /* tile 32×32                    */
    reg_tiled<  32,  8,  4,  1>();   /* tile  8×128                   */
    /* -- wavefront-aligned (BX=64) — key for MI300A peak --         */
    reg_tiled<  64,  2,  1,  1>();   /* tile  2×64                    */
    reg_tiled<  64,  4,  1,  1>();   /* tile  4×64                    */
    reg_tiled<  64,  4,  1,  2>();   /* tile  8×64                    */
    reg_tiled<  64,  4,  2,  1>();   /* tile  4×128                   */
    reg_tiled<  64,  4,  2,  2>();   /* tile  8×128                   */
    reg_tiled<  64,  4,  2,  4>();   /* tile 16×128                   */
    reg_tiled<  64,  8,  1,  1>();   /* tile  8×64                    */
    reg_tiled<  64,  8,  1,  2>();   /* tile 16×64                    */
    reg_tiled<  64,  8,  2,  1>();   /* tile  8×128                   */
    reg_tiled<  64,  8,  2,  2>();   /* tile 16×128                   */
    reg_tiled<  64, 16,  1,  1>();   /* tile 16×64                    */
    reg_tiled<  64, 16,  2,  1>();   /* tile 16×128                   */
    /* -- wider blocks --                                             */
    reg_tiled< 128,  2,  1,  1>();   /* tile  2×128                   */
    reg_tiled< 128,  2,  2,  2>();   /* tile  4×256                   */
    reg_tiled< 128,  4,  1,  1>();   /* tile  4×128                   */
    reg_tiled< 128,  4,  1,  2>();   /* tile  8×128                   */
    reg_tiled< 128,  4,  2,  2>();   /* tile  8×256                   */
    reg_tiled< 128,  8,  1,  1>();   /* tile  8×128                   */
    reg_tiled< 256,  2,  1,  1>();   /* tile  2×256                   */
    reg_tiled< 256,  4,  1,  1>();   /* tile  4×256                   */

    /* ==== all row-major control (peak) — broaden so the reported ==
     *      peak doesn't hinge on a single (BX,BY) shape. */
    reg_control<  32,  8,  1,  1>();
    reg_control<  32,  8,  4,  4>();
    reg_control<  16, 16,  1,  1>();
    reg_control<  32, 32,  1,  1>();
    reg_control<  32,  8,  2,  2>();
    reg_control<  32,  4,  1,  1>();
    reg_control<  32,  4,  2,  2>();
    reg_control<  64,  1,  1,  1>();
    reg_control<  64,  4,  1,  1>();
    reg_control<  64,  4,  2,  2>();
    reg_control<  64,  8,  1,  1>();
    reg_control<  64,  8,  2,  2>();
    reg_control<  64, 16,  1,  1>();
    reg_control< 128,  4,  1,  1>();
    reg_control< 128,  4,  2,  2>();
    reg_control< 128,  8,  1,  1>();
    reg_control< 256,  2,  1,  1>();
    reg_control< 256,  4,  1,  1>();
}


/*
 *  Per-iteration recording for CSV
 * ================================================================ */
struct IterRecord {
    std::string kernel;
    int tile_rows, tile_cols;
    int rep;
    float time_ms;
    double bw_gbs;
    double checksum;
    bool ok;
};

static std::vector<IterRecord> g_iter_records;

/* Summary per kernel (for console + final table) */
struct Result {
    std::string name;
    double median_ms;
    double bw_GBs;
    bool ok;
};

static Result run_bench(const KernelConfig& cfg,
                        const double* dA, const double* dB_cm,
                        const double* dB_rm, double* dC,
                        double* hC, double ref_cs, double ref_cs_rm)
{
    const size_t total = (size_t)Md * Nd;
    const double data_bytes = (double)total * sizeof(double) * 4.0;  /* 2R + 1RW (C+=) */

    const double* dB = cfg.uses_B_rm ? dB_rm : dB_cm;
    double expected_cs = cfg.uses_B_rm ? ref_cs_rm : ref_cs;

    /* check smem limit against the opt-in cap (H100/H200: 227 KiB,       */
    /* MI300A: 64 KiB) and opt the kernel in via cudaFuncSetAttribute     */
    /* if the requested size exceeds the default cap. Only then do we    */
    /* genuinely skip.                                                    */
    if (cfg.smem_bytes > 0 && cfg.kernel_fn) {
        gpuDeviceProp prop;
        GPU_CHECK(gpuGetDeviceProperties(&prop, 0));
        const size_t hw_limit = gpu_max_dynamic_smem_per_block(prop);
        if (cfg.smem_bytes > hw_limit) {
            printf("  %-62s  SKIP (smem %zu > %zu hw limit)\n",
                   cfg.name.c_str(), cfg.smem_bytes, hw_limit);
            return {cfg.name, 0, 0, false};
        }
        if (!gpu_opt_in_max_dynamic_smem(cfg.kernel_fn, cfg.smem_bytes, prop)) {
            printf("  %-62s  SKIP (smem opt-in failed at %zu B)\n",
                   cfg.name.c_str(), cfg.smem_bytes);
            return {cfg.name, 0, 0, false};
        }
    }

    /* warmup */
    for (int w = 0; w < NWARMUP; w++) {
        GPU_CHECK(gpuMemset(dC, 0, total * sizeof(double)));
        cfg.launch(dA, dB, dC);
    }
    GPU_CHECK(gpuDeviceSynchronize());

    /* correctness */
    GPU_CHECK(gpuMemcpy(hC, dC, total * sizeof(double), gpuMemcpyDeviceToHost));
    double cs = 0.0;
    for (size_t k = 0; k < total; k++) cs += hC[k];
    bool ok = (std::fabs(cs - expected_cs) <= 1e-3 * std::fabs(expected_cs));
    if (!ok)
        fprintf(stderr, "  [%s] CHECKSUM MISMATCH: got %.6e expected %.6e\n",
                cfg.name.c_str(), cs, expected_cs);

    /* timed runs -- record every iteration */
    gpuEvent_t t0, t1;
    GPU_CHECK(gpuEventCreate(&t0));
    GPU_CHECK(gpuEventCreate(&t1));

    float times[NREP];
    for (int r = 0; r < NREP; r++) {
        GPU_CHECK(gpuMemset(dC, 0, total * sizeof(double)));
        GPU_CHECK(gpuDeviceSynchronize());
        GPU_CHECK(gpuEventRecord(t0));
        cfg.launch(dA, dB, dC);
        GPU_CHECK(gpuEventRecord(t1));
        GPU_CHECK(gpuEventSynchronize(t1));
        GPU_CHECK(gpuEventElapsedTime(&times[r], t0, t1));

        double bw_r = data_bytes / ((double)times[r] * 1e-3) / 1e9;
        g_iter_records.push_back({cfg.name, cfg.tile_rows, cfg.tile_cols,
                                  r, times[r], bw_r, cs, ok});
    }
    GPU_CHECK(gpuEventDestroy(t0));
    GPU_CHECK(gpuEventDestroy(t1));

    std::sort(times, times + NREP);
    double med = times[NREP / 2];
    double bw  = data_bytes / ((double)med * 1e-3) / 1e9;

    printf("  %-62s  %8.3f ms  %8.1f GB/s  %s\n",
           cfg.name.c_str(), med, bw, ok ? "OK" : "FAIL");

    return {cfg.name, med, bw, ok};
}


/* ================================================================ */
int main(int argc, char** argv)
{
    const char* csv_path = (argc > 1) ? argv[1] : nullptr;

    gpuDeviceProp prop;
    GPU_CHECK(gpuGetDeviceProperties(&prop, 0));
    printf("Layout-conflict benchmark (GPU: %s)\n", prop.name);
    printf("  M=%d  N=%d  reps=%d\n", Md, Nd, NREP);
    printf("  A: row-major   B: col-major   C: row-major\n");
    printf("  Warp=32  L2-line=128B → %d doubles/txn\n\n",
           128 / (int)sizeof(double));

    const size_t total = (size_t)Md * Nd;
    const size_t bytes = total * sizeof(double);

    double *dA, *dB_cm, *dB_rm, *dC;
    GPU_CHECK(gpuMalloc(&dA,    bytes));
    GPU_CHECK(gpuMalloc(&dB_cm, bytes));
    GPU_CHECK(gpuMalloc(&dB_rm, bytes));
    GPU_CHECK(gpuMalloc(&dC,    bytes));

    double* hC = (double*)malloc(bytes);
    if (!hC) { fprintf(stderr, "host malloc failed\n"); return 1; }

    /* init all arrays */
    {
        int nblk = ((int)total + 255) / 256;
        init_kernel<<<nblk, 256>>>(dA, dB_cm, dB_rm);
        GPU_CHECK(gpuDeviceSynchronize());
    }

    /* reference checksum for col-major B */
    GPU_CHECK(gpuMemset(dC, 0, bytes));
    {
        dim3 blk(32, 8);
        dim3 grd((Nd + 31) / 32, (Md + 7) / 8);
        kernel_direct<32, 8, 1, 1><<<grd, blk>>>(dA, dB_cm, dC);
        GPU_CHECK(gpuDeviceSynchronize());
    }
    GPU_CHECK(gpuMemcpy(hC, dC, bytes, gpuMemcpyDeviceToHost));
    double ref_cs = 0.0;
    for (size_t k = 0; k < total; k++) ref_cs += hC[k];

    /* reference checksum for row-major B (control) */
    GPU_CHECK(gpuMemset(dC, 0, bytes));
    {
        dim3 blk(32, 8);
        dim3 grd((Nd + 31) / 32, (Md + 7) / 8);
        kernel_control<32, 8, 1, 1><<<grd, blk>>>(dA, dB_rm, dC);
        GPU_CHECK(gpuDeviceSynchronize());
    }
    GPU_CHECK(gpuMemcpy(hC, dC, bytes, gpuMemcpyDeviceToHost));
    double ref_cs_rm = 0.0;
    for (size_t k = 0; k < total; k++) ref_cs_rm += hC[k];

    /* register and run all configs */
    register_configs();

    printf("%-64s  %10s  %10s\n", "Kernel", "Median", "BW");
    printf("%s\n", std::string(96, '-').c_str());

    std::vector<Result> results;
    for (auto& cfg : g_configs)
        results.push_back(run_bench(cfg, dA, dB_cm, dB_rm, dC, hC, ref_cs, ref_cs_rm));

    /* ---- summary ---- */
    double best_tiled = 1e30, best_direct = 1e30, best_direct_T = 1e30;
    double best_ctrl = 1e30;
    for (auto& r : results) {
        if (!r.ok || r.median_ms == 0) continue;
        if (r.name.find("tiled") != std::string::npos)
            best_tiled = std::min(best_tiled, r.median_ms);
        else if (r.name.find("direct_T") != std::string::npos)
            best_direct_T = std::min(best_direct_T, r.median_ms);
        else if (r.name.find("direct") != std::string::npos)
            best_direct = std::min(best_direct, r.median_ms);
        else if (r.name.find("all_rowmajor") != std::string::npos)
            best_ctrl = std::min(best_ctrl, r.median_ms);
    }

    printf("\n=== Summary (best of each category) ===\n");
    if (best_tiled < 1e30)
        printf("  Best tiled+smem : %.3f ms  (baseline)\n", best_tiled);
    if (best_direct < 1e30)
        printf("  Best direct     : %.3f ms  (%.1fx vs tiled)  — B uncoalesced\n",
               best_direct, best_direct / best_tiled);
    if (best_direct_T < 1e30)
        printf("  Best direct_T   : %.3f ms  (%.1fx vs tiled)  — A,C uncoalesced\n",
               best_direct_T, best_direct_T / best_tiled);
    if (best_ctrl < 1e30)
        printf("  Best control    : %.3f ms  (%.2fx vs tiled)  — all row-major\n",
               best_ctrl, best_ctrl / best_tiled);

    const double data_bytes = (double)total * sizeof(double) * 4.0;  /* 2R + 1RW (C+=) */
    printf("\n  Peak BW (best control): %.1f GB/s\n",
           data_bytes / (best_ctrl * 1e-3) / 1e9);
    printf("  Tiled recovery:         %.1f GB/s\n",
           data_bytes / (best_tiled * 1e-3) / 1e9);

    /* CSV dump — one row per iteration for violin plots */
    if (csv_path) {
        FILE* fp = fopen(csv_path, "w");
        if (fp) {
            fprintf(fp, "kernel,M,N,tile_rows,tile_cols,rep,time_ms,bw_gbs,checksum,status\n");
            for (auto& ir : g_iter_records)
                fprintf(fp, "\"%s\",%d,%d,%d,%d,%d,%.6f,%.3f,%.6e,%s\n",
                        ir.kernel.c_str(), Md, Nd, ir.tile_rows, ir.tile_cols,
                        ir.rep, (double)ir.time_ms, ir.bw_gbs, ir.checksum,
                        ir.ok ? "PASS" : "FAIL");
            fclose(fp);
            printf("\nWrote %zu records to %s\n", g_iter_records.size(), csv_path);
        }
    }

    GPU_CHECK(gpuFree(dA));
    GPU_CHECK(gpuFree(dB_cm));
    GPU_CHECK(gpuFree(dB_rm));
    GPU_CHECK(gpuFree(dC));
    free(hC);
    return 0;
}