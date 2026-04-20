# E3 — Matrix Transpose (Figure 9, Table III)

`N × N` fp32 transpose under row-major and blocked layouts.
Compares DaCe-produced variants against vendor libraries:
HPTT (CPU), cuTENSOR (Daint GPU), hipTensor (Beverin GPU).

## How to run

```bash
# 1. One-time (from repo root)
bash ../common/setup.sh

# 2. Submit on your cluster
sbatch run_daint.sh      # Daint.Alps   (Grace + Hopper)
sbatch run_beverin.sh    # Beverin      (Zen4 + MI300A)

# 3. Plot once both platforms finish
python plot_paper.py
python delta_table.py       # Table III (block-distance reduction)
```

`run_*.sh` sources `../common/activate.sh` then
`../common/setup_{daint,beverin}.sh` (which loads OpenBLAS: 0.3.29 on
Daint, 0.3.30 on Beverin), builds the CPU / GPU benchmarks and the
vendor-library baselines, runs the full `N` sweep, and then invokes
`cost_metrics` (task T3) to compute µ and Δ for each candidate
layout. CSVs are written under `results/{daint,beverin}/`.

## Files

- `transpose_cpu.cpp`, `transpose_gpu.cu`, `transpose_gpu_hip.cpp` —
  DaCe-produced transpose variants.
- `transpose_hptt.cpp` — HPTT CPU baseline.
- `transpose_cutensor.cu` — cuTENSOR baseline (Daint only).
- `transpose_hiptensor.cpp` — hipTensor baseline (Beverin only).
- `transpose_openblas.cpp` — OpenBLAS CPU baseline.
- `cost_metrics.cpp` — computes µ and Δ per layout candidate.
- `plot_paper.py` / `delta_table.py` — Figure 9 / Table III.

## Protocol

100 repetitions after 5 warm-ups with a Jacobi cache flush. Bootstrap
95% CIs of the median.

## Outputs

- `results/{daint,beverin}/transpose_{cpu,gpu}_{raw,results}.csv`.
- `transpose_metrics_{cpu,gpu}.csv` — µ and Δ per layout candidate.
- `transpose_paper.pdf` (Figure 9), `metrics_table.tex` (Table III).
