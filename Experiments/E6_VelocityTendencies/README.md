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
| T6.5 Full-module sweep | [`full_velocity_tendencies/`](full_velocity_tendencies/) | `sbatch run_{daint,beverin}.sh` *(TODO)* |
| T6.6 Plots / Table IV | root | `python plot_paper.py` |

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

- ✅ `loopnest_1` (indirect stencil `z_v_grad_w`) — complete.
- 🚧 `loopnest_{2..6}` — DaCe program stubs; C++/CUDA benchmarks TODO.
- 🚧 `conflict_resolution` — script TODO.
- 🚧 `full_velocity_tendencies` — lives in R2, currently a stub here.

## Outputs

- `loopnest_1/results/{daint,beverin}/z_v_grad_w_{cpu,gpu}.csv`
  (Figure 12, Table IV row 1).
- Once complete: `full_velocity_tendencies/results/{daint,beverin}/`
  drives Figure 13.
