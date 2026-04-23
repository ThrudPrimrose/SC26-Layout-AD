# E6 / Conflict Resolution (Task T6.4)

Documents how the per-loopnest rankings produced by `loopnest_{1..6}/`
are reduced into a single globally-selected layout set that is fed
forward into `full_velocity_tendencies/` (Task T6.5).

## Reduction recipe

For every pattern class, each `loopnest_{i}` contributes a ranked list
of candidate layouts (the CSVs under `loopnest_{i}/results/`). The
global ranking is formed by summing per-loopnest scores weighted by
call frequency in the two production configurations:

- Config A (coarse grid) : each loopnest contributes with weight 1/6.
- Config B (fine grid)   : each loopnest contributes with weight 5/6.

Where two loopnests disagree on the preferred layout for a shared
array group, the weighted score resolves the tie. The result --- the
cross-product of locally-optimal layouts per array group --- is
committed as `selected_layouts.json`.

## Files

- `selected_layouts.json` --- frozen output, consumed directly by
  `../full_velocity_tendencies/scripts/run_stage{4,8}_permutations.py`.
- `README.md` (this file) --- the recipe above; no runtime script is
  required to reproduce the reduction, the mapping is stable for the
  paper submission.

## Inputs (for audit)

`../loopnest_{1..6}/results/{daint,beverin}/*.csv`
