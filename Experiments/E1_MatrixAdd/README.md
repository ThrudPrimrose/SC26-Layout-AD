# E1 — Matrix Addition (Figure 4)

Elementwise `C += A + B` with `B` column-major. Measures the gap
between schedule-only tuning and full layout transformation.

**Expected result.** GPU reaches ~99% of STREAM peak after layout
permute on both Daint H200 and Beverin MI300A; CPU 68–88%.
Schedule-only baselines: CPU 36–72%, GPU 78–95%.

## Run

```bash
bash ../common/setup.sh          # one-time per machine
sbatch run_daint.sh              # or run_beverin.sh
python plot_paper.py --add-peak
```

## Files

- `bench_cpu.cpp` — NUMA-aware OpenMP CPU (variants: `row_major`,
  `col_major`, `tiled_T`, `all_rowmajor`; blocked: `blk_all_rm`,
  `blk_conflict`).
- `bench_gpu.cu` — GPU benchmark (CUDA and HIP; `bench_gpu_hip.cpp` is a thin shim, see [../README.md](../README.md#single-source-cudahip-pattern)).
- `plot_paper.py` — Figure 4.

## Protocol

100 reps after 5 warm-ups, 8192² Jacobi cache flush between reps,
BCa bootstrap 95 % CI (n=10 000).

## Outputs

- `results/{daint,beverin}/madd_{daint,beverin}_{cpu,gpu}.csv`
  (columns: variant, layout, rep, time_ms, bandwidth_GBs).
- `addition_paper_cpu_gpu{,_w_peak}.pdf` (Figure 4).

## Data loading

None — inputs synthesized in-process.

## Reviewer hint — `# TODO: VERSION`

Inherits common pins (see
[`../README.md`](../README.md#reviewer-hint----todo-version)).
