# E4 — Gather-Accumulate-Scatter (Figure 10)

Indirect scatter-accumulate pattern `y[σ(i)] += x[i]` before and
after applying a **shuffle** (index sorting). Evaluated under both
a uniform-random index distribution and the exact BaTiO₃
distribution.

## How to run

```bash
# 1. One-time (from repo root)
bash ../common/setup.sh

# 2. Fetch the BaTiO3 index data
bash download_data.sh

# 3. Submit on your cluster
sbatch run_daint.sh      # Daint.Alps   (Grace + Hopper)
sbatch run_beverin.sh    # Beverin      (Zen4 + MI300A)

# 4. Plot once both platforms finish
python plot_paper.py
```

`run_*.sh` sources `../common/activate.sh` then
`../common/setup_{daint,beverin}.sh`, builds `zaxpy` CPU/GPU kernels,
runs the indirect-access sweep under both distributions, and writes
CSVs under `results/{daint,beverin}/`.

## Files

- `zaxpy.cpp` — CPU benchmark (OpenMP).
- `zaxpy.cu` — CUDA GPU benchmark.
- `zaxpy_hip.cpp` — HIP GPU benchmark.
- `zaxpy_indirect_sweep/` — pre-materialised index distributions.
- `download_data.sh` — fetches additional BaTiO₃ indices if missing.
- `plot_paper.py` — produces Figure 10 from per-platform CSVs.

## Protocol

100 repetitions after 5 warm-ups with a Jacobi cache flush between
iterations. Bootstrap 95% CIs of the median.

## Outputs

- `results/{daint,beverin}/zaxpy_*.csv`.
- `zaxpy_violins_1gb.pdf` / `zaxpy_violins_small.pdf` (Figure 10).
