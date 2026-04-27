# E5 — QE `addusxx_g` (Figure 11, Listing 1)

Composed unzip + permute + shuffle applied to Quantum ESPRESSO's
`addusxx_g` kernel on a 10-atom BaTiO₃ HSE06 SCF calculation.

**Expected behaviour.** Composed transformation should match or beat
the QE baseline on every backend; the gain should be largest on Hopper
(paper: ~1.18× Hopper, ~1.12× Zen4, ~1.11× Grace, ~1.00× MI300A).

## Run

```bash
bash ../common/setup.sh          # one-time per machine
bash download_data.sh            # ≈ 1 GB BaTiO₃ state
sbatch run_daint.sh              # or run_beverin.sh
python plot_paper.py
```

## Files

- `main_cpu.cpp` — CPU driver (AoS / SoA variants).
- `main.cu` — GPU driver (CUDA and HIP; `main_hip.cpp` is a thin shim, see [../README.md](../README.md#single-source-cudahip-pattern)).
- `usxx_kernels*.{h,cu,cpp}` — baseline AoS + transformed SoA kernels.
- `data_loading.h`, `types.h` — shared data-format definitions.
- `download_data.sh` — fetches `bin/` from ETH PolyBox.
- `plot_paper.py` — Figure 11.

## Protocol

100 reps after 5 warm-ups, Jacobi cache flush, bootstrap 95 % CI of
the median.

## Outputs

- `results/{daint,beverin}/addusxx_*.csv`.
- `addusxx_sweep.pdf` (Figure 11).

## Data loading

- `download_data.sh` — ≈ 1 GB serialized BaTiO₃ state, outbound HTTPS
  (ETH PolyBox); writes into `bin/`. Run from this folder.

## Reviewer hint — `# TODO: VERSION`

Inherits common pins (see
[`../README.md`](../README.md#reviewer-hint----todo-version)).
Experiment-specific: the dataset URL inside `download_data.sh` —
override with `USXX_DATA_URL=<mirror> bash download_data.sh` if PolyBox
moves.
