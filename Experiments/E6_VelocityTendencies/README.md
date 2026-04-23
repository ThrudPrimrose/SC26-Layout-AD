# E6 — ICON Velocity Tendencies (Figures 12–13, Table IV)

End-to-end optimization of the ICON velocity-tendencies module (25
loop nests, 50 arrays): canonicalize → score with µ/Δ → per-class
benchmark → resolve conflicts → full-module sweep.

Uses **two DaCe branches**: `yakup/dev` everywhere except
`full_velocity_tendencies/` which needs `f2dace/staging`.

## Pipeline

| Task | Folder | Script |
|---|---|---|
| T6.1 Data | `loopnest_1/` | `bash download_data.sh` |
| T6.2 Access analysis | [`access_analysis/`](access_analysis/) | `python analyze_loopnests.py` |
| T6.3 Per-loopnest sweep | [`loopnest_{1..6}/`](loopnest_1/) | `sbatch run_{daint,beverin}.sh` |
| T6.4 Conflict resolution *(TODO)* | [`conflict_resolution/`](conflict_resolution/) | `python resolve_conflicts.py` |
| T6.5 Full-module sweep | [`full_velocity_tendencies/`](full_velocity_tendencies/) | `sbatch run_{daint,beverin}.sh` |
| T6.6 Plots | subfolders | `loopnest_1/plot.py` (Fig. 12 / Tab. IV); `full_velocity_tendencies/scripts/plotting/plot_stage5_8_combined.py` (Fig. 13) |

## Run a subtask

```bash
bash ../../common/setup.sh       # one-time (repo root)
cd loopnest_1                    # or any other subtask
sbatch run_daint.sh              # or run_beverin.sh
```

Full-module stage needs the other DaCe branch:

```bash
DACE_BRANCH=f2dace/staging source ../../common/activate.sh
```

## Status

- ✅ `loopnest_{1..6}` — C++ / CUDA / HIP benchmarks + `cost_metrics.cpp`.
- 🚧 `conflict_resolution` — aggregator TODO; layouts are currently
  hard-coded in `full_velocity_tendencies/scripts/run_stage{4,8}_permutations.py`.
- ✅ `full_velocity_tendencies` — stage4 / stage8 permutation sweep,
  mirrored from R2 (`spcl/icon-artifacts`, `f2dace/staging`).

## Outputs

- `loopnest_{1..6}/results/{daint,beverin}/*.csv` (Figure 12, Table IV).
- `full_velocity_tendencies/results/{daint,beverin}/stage{4,8}/*.csv`
  (Figure 13).

## Data loading

- `loopnest_1/download_data.sh` — ICON R02B05 serialized input (≈ 3 GB),
  shared across **all** `loopnest_{1..6}`.
- `full_velocity_tendencies/scripts/download_nproma20480_data.sh` —
  full-module `nproma=20480` dataset (≈ 8 GB).

Outbound HTTPS required; ≈ 11 GB scratch quota.

## Reviewer hint — `# TODO: VERSION`

Common pins — see
[`../README.md`](../README.md#reviewer-hint----todo-version). E6-specific:

- **DaCe branch** — `yakup/dev` vs. `f2dace/staging` (see above).
- **Dataset URLs** in the two `download_data.sh`'s (override with
  `DATA_URL=<mirror>`).
- **ICON commit** used to serialize inputs — pin the same SHA if you
  rebuild the dataset, or outputs won't be bit-identical.
