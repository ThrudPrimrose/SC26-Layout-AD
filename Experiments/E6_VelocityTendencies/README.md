# E6 — ICON Velocity Tendencies, per-loopnest sweeps (Figures 12, Table IV)

Per-loopnest evaluation of the ICON velocity-tendencies module (25 loop
nests, 50 arrays): canonicalize → score with µ/Δ → per-class benchmark →
resolve conflicts.

The full-module GPU permutation sweep that used to live under
`full_velocity_tendencies/` has been promoted to its own experiment,
**[E7](../E7_FullVelocityTendencies/)**. E7 reads
[`access_analysis/layout_candidates.json`](access_analysis/layout_candidates.json)
from this folder, so run E6's task `T6.2` before E7.

E6 runs entirely on DaCe `yakup/dev`.

## Pipeline

| Task | Folder | Script |
|---|---|---|
| T6.1 Data | `loopnest_1/` | `bash download_data.sh` |
| T6.2 Access analysis | [`access_analysis/`](access_analysis/) | `python analyze_loopnests.py` |
| T6.3 Per-loopnest sweep | [`loopnest_{1..6}/`](loopnest_1/) | `sbatch run_{daint,beverin}.sh` |
| T6.4 Conflict resolution *(TODO)* | [`conflict_resolution/`](conflict_resolution/) | `python resolve_conflicts.py` |
| T6.5 Plots | subfolders | `loopnest_1/plot.py` (Fig. 12 / Tab. IV) |

The full-module sweep (former T6.5/T6.6) is now
[E7](../E7_FullVelocityTendencies/) — see its README.

## Run a subtask

```bash
bash ../../common/setup.sh       # one-time (repo root)
cd loopnest_1                    # or any other subtask
sbatch run_daint.sh              # or run_beverin.sh
```

## Status

- ✅ `access_analysis/` — produces `layout_candidates.json` consumed by E7.
- ✅ `loopnest_{1..6}` — CPU + GPU benchmarks (single-source CUDA/HIP via `common/gpu_compat.cuh`) + `cost_metrics.cpp`.
- 🚧 `conflict_resolution` — aggregator TODO.

## Outputs

- `loopnest_{1..6}/results/{daint,beverin}/*.csv` (Figure 12, Table IV).

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
