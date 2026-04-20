# E1 — Matrix Addition (Figure 4)

Elementwise `C += A + B` with `B` stored in column-major layout.
Measures how far schedule-only tuning can close the gap to STREAM
peak vs. applying layout transformations.

## How to run

```bash
# 1. One-time (from repo root)
bash ../common/setup.sh

# 2. Submit on your cluster
sbatch run_daint.sh      # Daint.Alps   (Grace + Hopper)
sbatch run_beverin.sh    # Beverin      (Zen4 + MI300A)

# 3. Plot once both platforms finish
python plot_paper.py --add-peak
```

`run_*.sh` sources `../common/activate.sh` (pyenv venv + DaCe
`yakup/dev`) then `../common/setup_{daint,beverin}.sh` (spack modules,
OMP pinning, `CPU_*`/`GPU_*` flags), builds `bench_cpu` and
`bench_gpu`, runs both, and writes CSVs under `results/{daint,beverin}/`.

## Files

- `bench_cpu.cpp` — NUMA-aware OpenMP CPU benchmark.
  Schedule variants: `row_major`, `col_major`, `tiled_T`,
  `all_rowmajor`. Blocked-layout variants: `blk_all_rm`, `blk_conflict`.
- `bench_gpu.cu` — CUDA GPU benchmark (Daint / Hopper).
- `bench_gpu_hip.cpp` — HIP GPU benchmark (Beverin / MI300A).
- `plot_paper.py` — produces Figure 4 from per-platform CSVs.

## Protocol

100 repetitions after 5 warm-ups, with an 8192² Jacobi cache flush
between iterations (enforced inside `bench_cpu.cpp` /
`bench_gpu.cu`). Bootstrap 95% CIs of the median (BCa, n=10 000).

## Outputs

- `results/{daint,beverin}/madd_{daint,beverin}_{cpu,gpu}.csv`
  (columns: variant, layout, rep, time_ms, bandwidth_GBs).
- `addition_paper_cpu_gpu.pdf` / `addition_paper_cpu_gpu_w_peak.pdf`
  (Figure 4).
