# E2 — Complex Conjugation (Figure 8)

In-place complex-conjugate of `P` arrays under three layouts:
**AoS** (interleaved re/im per element), **SoA** (separate real and
imaginary streams), and **AoSoA-16** (partial zip with inner block
size 16). Sweeps `P` to expose L1D set-conflict behaviour on CPUs
with limited associativity.

## How to run

```bash
# 1. One-time (from repo root)
bash ../common/setup.sh

# 2. Submit on your cluster
sbatch run_daint.sh      # Daint.Alps   (Grace + Hopper)
sbatch run_beverin.sh    # Beverin      (Zen4 + MI300A)

# 3. Plot once both platforms finish
python plot_paper.py
```

`run_*.sh` sources `../common/activate.sh` then
`../common/setup_{daint,beverin}.sh`, builds the CPU and GPU
benchmarks, runs them over the full `P` sweep, and writes CSVs under
`results/{daint,beverin}/`.

## Files

- `conjugate_inplace.cpp` / `conjugate_inplace.cu` /
  `conjugate_inplace_hip.cpp` — the three primary in-place kernels
  exercised by the AD.
- `conjugate.cpp` / `.cu` / `_hip.cpp` — out-of-place variants for
  comparison.
- `plot_paper.py` — produces Figure 8 from per-platform CSVs.

## Protocol

100 repetitions after 5 warm-ups with a Jacobi cache flush between
iterations. Bootstrap 95% CIs of the median.

## Outputs

- `results/{daint,beverin}/results_{cpu,gpu}_inplace.csv`
  and `_oop.csv` (columns: layout, P, rep, time_ms, bandwidth_GBs).
- `conjugate_inplace.pdf` / `conjugate_oop.pdf` (Figure 8).
