# E6 — ICON Velocity Tendencies (Figures 12–13, Table IV)

End-to-end optimization of the ICON velocity-tendencies module
(25 loop nests, 50 arrays) using the paper's methodology:
canonicalize loop nests into pattern classes, score candidate
layouts with µ/Δ, benchmark per-class, resolve cross-nest conflicts,
then evaluate the full module.

This experiment spans **two repositories and two DaCe branches**:

| Sub-task | DaCe branch | Home |
|---|---|---|
| access_analysis, loopnest_{1..6}, conflict_resolution | `yakup/dev` | this repo |
| full_velocity_tendencies | `f2dace/staging` | R2 (`spcl/icon-artifacts`) |

Switch DaCe branches explicitly when running the full-module stage:

```bash
DACE_BRANCH=f2dace/staging source ../common/activate.sh
```

## Pipeline

| Task | Folder | Script |
|---|---|---|
| T6.1 Download data | `loopnest_1/` | `bash download_data.sh` |
| T6.2 Access analysis | [`access_analysis/`](access_analysis/) | `python analyze_loopnests.py` |
| T6.3 Per-loopnest sweep | [`loopnest_{1..6}/`](loopnest_1/) | `sbatch run_{daint,beverin}.sh` |
| T6.4 Conflict resolution | [`conflict_resolution/`](conflict_resolution/) | `python resolve_conflicts.py` *(TODO)* |
| T6.5 Full-module sweep | [`full_velocity_tendencies/`](full_velocity_tendencies/) | `sbatch run_{daint,beverin}.sh` |
| T6.6 Plots / Table IV | subfolders | [`loopnest_1/plot.py`](loopnest_1/plot.py) drives Fig. 12 / Tab. IV; [`full_velocity_tendencies/scripts/plotting/plot_stage5_8_combined.py`](full_velocity_tendencies/scripts/plotting/plot_stage5_8_combined.py) drives Fig. 13 |

## How to run any subtask

```bash
# 1. One-time (from repo root)
bash ../../common/setup.sh

# 2. Enter the subtask folder
cd loopnest_1    # or any other

# 3. Submit on your cluster
sbatch run_daint.sh
sbatch run_beverin.sh
```

Each `run_*.sh` sources `../../common/activate.sh` then
`../../common/setup_{daint,beverin}.sh` (matching the two-level depth).

## Status

- ✅ `loopnest_1` (indirect stencil `z_v_grad_w`) — full C++ / CUDA / HIP
  benchmark suite + analytic cost metrics.
- ✅ `loopnest_{2..6}` — complete C++ / CUDA / HIP benchmarks and
  matching `cost_metrics.cpp`. Classes: direct stencil partial-vert,
  direct stencil full-vert, indirect stencil + CFL clip, horizontal-only
  boundary, vertical-only level reduction.
- 🚧 `conflict_resolution` — per-loopnest ranking aggregator is still a
  TODO; layouts selected for the full-module sweep are hard-coded in
  `full_velocity_tendencies/scripts/run_stage{4,8}_permutations.py`.
- ✅ `full_velocity_tendencies` — stage4 / stage8 layout-permutation
  sweep driver + baseline SDFGs + DaCe transformation utilities,
  lifted from R2 (`spcl/icon-artifacts`, branch `f2dace/staging`) and
  mirrored here for self-contained reproduction.

## Outputs

- `loopnest_{1..6}/results/{daint,beverin}/*.csv` — per-loopnest
  timing data (Figure 12, Table IV).
- `full_velocity_tendencies/results/{daint,beverin}/stage{4,8}/*.csv`
  — layout-permutation sweep for Figure 13.
