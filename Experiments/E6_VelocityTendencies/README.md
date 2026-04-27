# E6 — ICON Velocity Tendencies, per-loopnest sweeps (Figures 12, Table IV)

Per-loopnest evaluation of the ICON velocity-tendencies module (25 loop
nests, 50 arrays): canonicalize → score with µ/Δ → per-class benchmark →
collapse to per-access-group winners → emit the winners cross-product
that drives E7.

The full-module GPU permutation sweep that used to live under
`full_velocity_tendencies/` has been promoted to its own experiment,
**[E7](../E7_FullVelocityTendencies/)**. E7 reads
[`access_analysis/layout_candidates.json`](access_analysis/layout_candidates.json)
**and** [`full_velocity_tendencies/layout_crossproduct_winners.json`](full_velocity_tendencies/)
from this folder, so run E6's tasks `T6.2` and `T6.4` before submitting E7.

E6 runs entirely on DaCe `yakup/dev`.

## Pipeline

| Task | Folder | Script |
|---|---|---|
| T6.1 Data | `loopnest_1/` | `bash download_data.sh` |
| T6.2 Access analysis | [`access_analysis/`](access_analysis/) | `python analyze_loopnests.py` |
| T6.3 Per-loopnest sweep | [`loopnest_{1..6}/`](loopnest_1/) | `sbatch run_{daint,beverin}.sh` |
| T6.4 Winners cross-product | this folder | `python generate_winners.py` |
| T6.5 Plots | subfolders | `loopnest_1/plot_paper.py` (Fig. 12 / Tab. IV) |

The full-module sweep (former T6.5/T6.6) is now
[E7](../E7_FullVelocityTendencies/) — see its README.

## T6.4 — Winners cross-product

[`generate_winners.py`](generate_winners.py) reads each
`loopnest_{1..6}/results/{beverin,daint}/<kernel>_<backend>.csv` at
the chosen `--nlev` slice (default 256, configurable), picks the
lowest-time non-blocked V-id per (loopnest, platform), and aggregates
the empirical winners into a V-id set per access group via
[`access_analysis/canonical_array_groups.json`](access_analysis/canonical_array_groups.json).

A **promotion rule** then widens narrow per-group sets so the
cross-product stays informative:

> Find the largest set `S` such that more than `--threshold` (default
> 50%) of the *non-empty* per-group winner sets are subsets of `S`. If
> such an `S` exists, every group whose winner set is a subset of `S`
> (including empty groups, i.e. groups not covered by any loopnest) is
> promoted to `S`.

The full cross-product over the promoted per-group sets is written in
the same schema as the older `layout_crossproduct_v123.json`, so
[`E7/tools/run_layout_configs.py`](../E7_FullVelocityTendencies/tools/run_layout_configs.py)
consumes it unchanged.

```bash
python generate_winners.py                          # nlev=256, gpu, exact, threshold 0.5
python generate_winners.py --nlev 90                # different vertical resolution
python generate_winners.py --no-promotion           # raw per-group winners only
python generate_winners.py --threshold 0.66         # tighter promotion bar
```

Outputs:

- [`full_velocity_tendencies/layout_crossproduct_winners.json`](full_velocity_tendencies/) — full record (meta + per-group sets + cross product cells).
- [`full_velocity_tendencies/layout_crossproduct_winners.csv`](full_velocity_tendencies/) — one row per cell, with each group's V-id and IC/IN axis projection.

Companion reporter [`access_analysis/find_groups_and_loopnests.py`](access_analysis/find_groups_and_loopnests.py)
prints the 6 access groups (read from `canonical_array_groups.json`)
and the 6 manually-pinned representative loopnests so reviewers can
audit what's feeding the winners pipeline.

## Run a subtask

```bash
bash ../../common/setup.sh       # one-time (repo root)
cd loopnest_1                    # or any other subtask
sbatch run_daint.sh              # or run_beverin.sh
```

## Status

- ✅ `access_analysis/` — produces `layout_candidates.json` + `canonical_array_groups.json` consumed by E7.
- ✅ `loopnest_{1..6}` — CPU + GPU benchmarks (single-source CUDA/HIP via `common/gpu_compat.cuh`) + `cost_metrics.cpp`.
- ✅ `generate_winners.py` — emits `full_velocity_tendencies/layout_crossproduct_winners.json` consumed by E7.

## Outputs

- `loopnest_{1..6}/results/{daint,beverin}/*.csv` (Figure 12, Table IV).
- `full_velocity_tendencies/layout_crossproduct_winners.{json,csv}` — winners cross-product fed to E7.

## Data loading

- `loopnest_1/download_data.sh` — ICON R02B05 serialized input (≈ 3 GB),
  shared across **all** `loopnest_{1..6}`.
- E7 has its own larger dataset; see
  [`../E7_FullVelocityTendencies/tools/download_data.sh`](../E7_FullVelocityTendencies/tools/download_data.sh).

Outbound HTTPS required; ≈ 3 GB scratch quota for E6 alone.

## Reviewer hint — `# TODO: VERSION`

Common pins — see
[`../README.md`](../README.md#reviewer-hint----todo-version). E6-specific:

- **Dataset URLs** in `loopnest_1/download_data.sh` (override with
  `DATA_URL=<mirror>`).
- **ICON commit** used to serialize inputs — pin the same SHA if you
  rebuild the dataset, or outputs won't be bit-identical.
