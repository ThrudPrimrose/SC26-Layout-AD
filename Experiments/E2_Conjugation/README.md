# E2 — Complex Conjugation (Figure 8)

In-place conjugate of `P` complex arrays under **AoS**, **SoA**, and
**AoSoA-16** layouts. Sweeps `P` to expose L1D set-conflict behaviour.

**Expected behaviour.** AoSoA-16 should be the most robust layout
across all four backends; SoA should degrade on CPUs at high `P` from
L1D set-conflict misses (paper: pronounced collapse on Zen4).

## Run

```bash
bash ../common/setup.sh          # one-time per machine
sbatch run_daint.sh              # or run_beverin.sh
python plot_paper.py
```

## Files

- `conjugate_inplace.{cpp,cu}` — primary in-place kernels (AoS / SoA / AoSoA-16); `conjugate_inplace_hip.cpp` is a thin shim.
- `conjugate.{cpp,cu}` — out-of-place variants for comparison; `conjugate_hip.cpp` is a thin shim.
- `plot_paper.py` — Figure 8.

## Protocol

100 reps after 5 warm-ups, Jacobi cache flush between reps,
bootstrap 95 % CI of the median.

## Outputs

- `results/{daint,beverin}/results_{cpu,gpu}_{inplace,oop}.csv`
  (columns: layout, P, rep, time_ms, bandwidth_GBs).
- `conjugate_{inplace,oop}.pdf` (Figure 8).

## Data loading

None — inputs synthesized in-process.

## Reviewer hint — `# TODO: VERSION`

Inherits common pins (see
[`../README.md`](../README.md#reviewer-hint----todo-version)).
