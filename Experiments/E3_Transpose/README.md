# E3 — Matrix Transpose (Figure 9, Table III)

`N × N` fp32 transpose, row-major vs. blocked layouts. DaCe-produced
variants vs. vendor libraries: HPTT (CPU), cuTENSOR (Daint GPU),
hipTensor (Beverin GPU).

**Expected result.** Blocking gains ~1.7× on Zen4 and ~3.2× on Grace
over row-major. Blocked transpose reaches 61–81% of STREAM peak vs
HPTT's ~44%. On GPUs shared-memory tiling absorbs the stride.

## Run

```bash
bash ../common/setup.sh          # one-time per machine
sbatch run_daint.sh              # or run_beverin.sh
python plot_paper.py
python delta_table.py            # Table III (block-distance reduction)
```

## Files

- `transpose_cpu.cpp`, `transpose_gpu.cu` — DaCe variants (single-source
  CUDA/HIP; `transpose_gpu_hip.cpp` is a thin shim that `#include`s the
  `.cu`).
- `transpose_hptt.cpp` — HPTT CPU baseline.
- `transpose_cutensor.cu` (Daint), `transpose_hiptensor.cpp` (Beverin)
  — vendor GPU baselines.
- `transpose_openblas.cpp` — OpenBLAS CPU baseline.
- `cost_metrics.cpp` — µ / Δ per layout candidate.
- `plot_paper.py`, `delta_table.py` — Figure 9, Table III.

## GPU variants

The `.cu` driver sweeps 10 kernel variants, all parameterised by
`(BX, BY, TX, TY)` and (where relevant) `SB`/`PAD`. V8 and V9 were
added specifically to close the gap to cuTENSOR / hipTensor on blocked
storage:

| # | Name | Notes |
|---|---|---|
| 0 | `naive`               | direct, scalar global loads/stores |
| 1 | `blocked`             | SB-tiled direct |
| 2 | `smem`                | shared-memory staging, no conflict fix |
| 3 | `smem_blk`            | smem + SB blocking |
| 4 | `smem_pad`            | smem + `BW+PAD` row stride to kill bank conflicts |
| 5 | `smem_swiz`           | smem + XOR swizzle |
| 6 | `smem_blk_swiz`       | smem + SB + XOR swizzle |
| 7 | `smem_pad_blk`        | smem + pad + SB (best scalar blocked so far) |
| 8 | `smem_pad_blk_vec`    | **V7 with `float4` (128-bit) global loads/stores** |
| 9 | `smem_pad_blk_vec_2x` | **V8 with each CTA processing 2 tiles along Y** (grid.y halved; compiler can hoist `load₂` ahead of `store₁` to hide DRAM latency — no explicit `cp.async`) |

V8 and V9 are the ones closing the blocked-storage gap with cuTENSOR /
hipTensor: V8 cuts issued memory instructions 4× for a pure bandwidth
gain, V9 layers a two-tile software pipeline on top.

## Protocol

100 reps after 5 warm-ups, Jacobi cache flush, bootstrap 95 % CI of
the median.

## Outputs

- `results/{daint,beverin}/transpose_{cpu,gpu}_{raw,results}.csv`.
- `transpose_metrics_{cpu,gpu}.csv` — µ, Δ per candidate.
- `transpose_paper.pdf` (Figure 9), `metrics_table.tex` (Table III).

## Data loading

None — `N × N` fp32 generated in-process.

## Reviewer hint — `# TODO: VERSION`

Version-sensitive beyond the common pins (see
[`../README.md`](../README.md#reviewer-hint----todo-version)):

- **HPTT**, **cuTENSOR** (Daint), **hipTensor** (Beverin) — loaded
  from spack by `setup_{daint,beverin}.sh`.
- **OpenBLAS** — 0.3.29 (Daint) / 0.3.30 (Beverin).

`cost_metrics.cpp` is vendor-independent; re-run the full sweep only
if a vendor library is upgraded.
