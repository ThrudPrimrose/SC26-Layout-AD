# E5 — Quantum ESPRESSO `addusxx_g` (Figure 11, Listing 1)

Composed layout transformations (unzip + permute + shuffle) applied
to the `addusxx_g` kernel from Quantum ESPRESSO's HSE06 self-consistent
calculation on a 10-atom BaTiO₃ unit cell.

## How to run

```bash
# 1. One-time (from repo root)
bash ../common/setup.sh

# 2. Fetch the serialized BaTiO3 data (~1 GB)
bash download_data.sh

# 3. Submit on your cluster
sbatch run_daint.sh      # Daint.Alps   (Grace + Hopper)
sbatch run_beverin.sh    # Beverin      (Zen4 + MI300A)

# 4. Plot once both platforms finish
python plot_paper.py
```

`run_*.sh` sources `../common/activate.sh` then
`../common/setup_{daint,beverin}.sh`, builds the baseline and
transformed kernel variants, runs them on the BaTiO₃ dataset, and
writes CSVs under `results/{daint,beverin}/`.

## Files

- `main_cpu.cpp` — CPU driver (AoS / SoA variants).
- `main.cu` — CUDA GPU driver.
- `main_hip.cpp` — HIP GPU driver.
- `usxx_kernels*.{h,cu,cpp}` — baseline AoS and transformed SoA
  kernel implementations (CPU / CUDA / HIP variants).
- `data_loading.h` / `types.h` — shared data-format definitions.
- `Makefile` — build rules consumed by `run_*.sh`.
- `download_data.sh` — fetches `bin/` contents from ETH PolyBox.
- `plot_paper.py` — produces Figure 11 from per-platform CSVs.

## Protocol

100 repetitions after 5 warm-ups with a Jacobi cache flush. Bootstrap
95% CIs of the median.

## Outputs

- `results/{daint,beverin}/addusxx_*.csv`.
- `addusxx_sweep.pdf` (Figure 11).
