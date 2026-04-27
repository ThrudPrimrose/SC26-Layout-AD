# E4 — Gather-Accumulate-Scatter (Figure 10)

Indirect scatter-accumulate `y[σ(i)] += x[i]` before and after an
index-sort shuffle. Evaluated under a uniform-random distribution and
under the real BaTiO₃ distribution.

**Expected behaviour.** Index-sort shuffle should lift bandwidth on
every backend; the gap should be largest on GPUs (paper: roughly
2× lift on MI300A and Hopper; CPU 9–15%).

## Run

```bash
bash ../common/setup.sh          # one-time per machine
bash download_data.sh            # ≈ 200 MB BaTiO₃ indices
sbatch run_daint.sh              # or run_beverin.sh
python plot_paper.py
```

## Files

- `zaxpy.cpp` — CPU (OpenMP).
- `zaxpy.cu` — GPU benchmark (CUDA and HIP; `zaxpy_hip.cpp` is a thin shim, see [../README.md](../README.md#single-source-cudahip-pattern)).
- `zaxpy_indirect_sweep/` — pre-materialised index distributions.
- `download_data.sh` — fetches BaTiO₃ indices if missing.
- `plot_paper.py` — Figure 10.

## Protocol

100 reps after 5 warm-ups, Jacobi cache flush, bootstrap 95 % CI of
the median.

## Outputs

- `results/{daint,beverin}/zaxpy_*.csv`.
- `zaxpy_violins_{small,1gb}.pdf` (Figure 10).

## Data loading

- `download_data.sh` — ≈ 200 MB BaTiO₃ index set, outbound HTTPS
  (ETH PolyBox), idempotent. Run from this folder.

## Reviewer hint — `# TODO: VERSION`

Inherits common pins (see
[`../README.md`](../README.md#reviewer-hint----todo-version)).
Experiment-specific: the dataset URL inside `download_data.sh` —
override with `E4_DATA_URL=<mirror> bash download_data.sh` if PolyBox
moves.
