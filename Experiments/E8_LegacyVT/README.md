# E8 — Full velocity tendencies (legacy pipeline, AD default)

> **Status: this is the AD's default reproduction path for §IV-D / Figure 14 / Table V.**
> The sibling [`../E7_FullVelocityTendencies/`](../E7_FullVelocityTendencies/)
> is a parallel work-in-progress reimplementation on the new SDFG-driven pipeline
> and is **not** required for the AD — reviewers should skip it and run E8.

GPU layout-permutation sweep on the full ICON velocity-tendencies module,
driving the legacy `icon-artifacts/sc26_layout` codegen pipeline on
DaCe `f2dace/staging`. Reproduces the V$_k$ winner comparison from
paper Fig 13 / 14 and Table V.

## Quick start

```bash
# 1. One-time setup (if not already done)
bash ../common/setup.sh

# 2. Submit (default = winner_v1, winner_v2, winner_v6)
cd Experiments/E8_LegacyVT
sbatch run_daint.sh           # or run_beverin.sh

# 3. After the job finishes, plot
python plot_paper.py
```

The `run_{daint,beverin}.sh` scripts handle:

- DaCe branch switch to `f2dace/staging` (E8 requires it).
- Dataset auto-fetch (R02B05 / `nproma=20480` ICON; ~9 GB). Three
  cases, in priority order:
  1. `${EXP_DIR}/data_r02b05/` already populated → use as-is.
  2. `../E7_FullVelocityTendencies/data_r02b05/` populated → symlink
     via `LOCAL_DATA_DIR` mode (no copy, no re-download).
  3. Otherwise → fetch via E7's `tools/download_data.sh`.
- Regeneration of E6's `layout_candidates.json` (via
  `select_loopnests.py`) and `winners.json` (via `derive_winners.py`)
  if either is missing.
- Compile + run dispatch through `run_stage8_permutations.py` per
  config in `CONFIGS`.

## Configs

The default `CONFIGS="winner_v1,winner_v2,winner_v6"` reproduces the
three V$_k$ winners exactly:

| Config | Mapping | Reproduces |
|---|---|---|
| `winner_v1` | identity (h_first + SoA-conn) | Fig 13 *Original Layout* |
| `winner_v2` | h_first + AoS-conn (`[0,2,1]` connectivity) | connectivity-only intermediate |
| `winner_v6` | v_first + AoS-conn (`[2,0,1]` connectivity, level-first data) | Fig 13 *Optimized Layout* / §IV-D adopted layout |

Override via the `CONFIGS` env var (comma-separated):

```bash
# Just V1 vs V6 (minimal Fig 13 reproduction)
CONFIGS="winner_v1,winner_v6" sbatch run_daint.sh

# Full 95-cell cross-product sweep
CONFIGS="$(python run_stage8_permutations.py --list \
          | awk 'NR>1 && /^[ ]/{print $1}' | head -95 \
          | paste -sd,)" sbatch run_daint.sh

# List all available named configs
python run_stage8_permutations.py --list
```

The 95-cell sweep is the cv × ch × f × s × n cross-product from
[`sc26_layout/permute_stage8.py`](sc26_layout/permute_stage8.py)
(2 × 2 × 2 × 2 × 6 - 1 = 95 non-identity cells), plus the three
named V$_k$ aliases.

## Outputs

Per-config TXTs (one per shuffle variant) land under:

```
{daint,beverin}_full_permutations_8/
├── winner_v1_shuffled.txt
├── winner_v1_unshuffled.txt
├── winner_v2_shuffled.txt
├── winner_v2_unshuffled.txt
├── winner_v6_shuffled.txt
├── winner_v6_unshuffled.txt
└── E8_velocity_<host>_<jobid>.{out,err}
```

Each `<config>_<shuffled|unshuffled>.txt` captures the binary's
stdout — `[Timer] Elapsed time: <ms>` lines plus reduction-correctness
asserts. The plotting scripts (`plot_paper.py`,
`Figures/plot_paper_snapshot.sh`) parse the timing lines via the same
regex used in `icon-artifacts/sc26_layout/plot_stage8.py`.

## Dependencies

| Component | Source | Notes |
|---|---|---|
| DaCe | `f2dace/staging` | Required for E8's codegen pipeline. The run script switches to it automatically; revert to `yakup/dev` after the job (or use a separate clone). |
| CUDA | **< 13** (12.9 verified) | CUDA 13 removes `cudaDeviceProp` fields DaCe's runtime probes during initialization, breaking E8's stage-8 codegen. Use CUDA 12.x. |
| ICON dataset | R02B05 / `nproma=20480` (~9 GB) | Auto-fetched / symlinked to `data_r02b05/`. Same dataset as E7. |
| `layout_candidates.json` | `../E6_VelocityTendencies/access_analysis/select_loopnests.py` | Regenerated on first run if missing. |
| `winners.json` | `../E6_VelocityTendencies/access_analysis/derive_winners.py` | Empirical V$_k$ winners per loopnest, from E6's loopnest CSVs. Regenerated on first run if missing. |
| `layout_groups.json` | `../E6_VelocityTendencies/access_analysis/generate_layout_groups.py` | The 7 layout-equivalent groups + V$_k$ permutation table. Read by `permute_stage8.py`'s named-alias builders. |

## Pipeline internals (skim)

- [`sc26_layout/permute_stage8.py`](sc26_layout/permute_stage8.py) —
  `PERMUTE_CONFIGS` dict (95 sweep cells + named aliases including
  `winner_v1` / `winner_v2` / `winner_v6`) and the post-codegen
  `permute_sdfg()` driver.
- [`run_stage8_permutations.py`](run_stage8_permutations.py) — main
  loop. Imports `PERMUTE_CONFIGS`, accepts `--configs` CSV /
  `--list` / `--reps`. Compiles per-config binaries via
  `compile_gpu_stage8`, runs each twice (shuffled / unshuffled),
  redirects stdout to the TXT files above.
- [`utils/stages/compile_gpu_stage8.py`](utils/stages/compile_gpu_stage8.py)
  — produces `velocity_gpu.stage8_standalone_release_permuted_<cfg>_<ms|mu>`
  binaries.
- `sc26_layout/permute_stage4.py` and `permute_stage8.py` are the two
  named entry points; E8 runs only the latter for the AD's full-VT
  result.

## Statistical methodology

Same as the rest of the AD: 100 timed reps after 5 warm-ups, Jacobi
cache flush between reps (`common/jacobi_flush.h`), no outlier
trimming, Scott-rule violin KDE with 200 eval points.

## Failure modes

- **Binary aborts during load (`std::out_of_range` from `map::at`)**:
  the run scripts no longer use `set -e` / `pipefail`, so a single
  config crash logs a `[E8 <plat>] WARN: ... aborted (rc=N); continuing`
  line and moves on to the next config / timestep. Cores and got/want
  blobs are cleaned up automatically (unless `--verify` was passed).
- **Missing `f2dace/staging` branch**: the run script tries to switch
  branches; if you've checked DaCe out somewhere it can't `git
  checkout` cleanly, fall back to a fresh clone:
  ```bash
  git clone -b f2dace/staging https://github.com/spcl/dace.git \
      ${EXP_DIR}/../common/dace_f2dace
  DACE_DIR=${EXP_DIR}/../common/dace_f2dace sbatch run_daint.sh
  ```
- **`layout_candidates.json` regeneration fails (LOKI not installed)**:
  ship the JSON manually — the file is committed under
  `../E6_VelocityTendencies/access_analysis/`.

## See also

- [`../E6_VelocityTendencies/`](../E6_VelocityTendencies/) — per-loopnest
  V$_k$ sweeps that feed E8's `winners.json`.
- [`../E6_VelocityTendencies/access_analysis/`](../E6_VelocityTendencies/access_analysis/)
  — the E6→E7/E8 audit chain (`layout_groups.json`, `winners.json`).
- [`../E7_FullVelocityTendencies/`](../E7_FullVelocityTendencies/) —
  WIP reimplementation; **skip for the AD**.
