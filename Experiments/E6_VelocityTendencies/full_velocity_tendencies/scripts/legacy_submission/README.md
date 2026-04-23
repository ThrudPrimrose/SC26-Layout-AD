# Legacy submission scripts

These are the raw SLURM scripts as they lived in the original
`icon-artifacts/velocity` working tree on the `sc26` branch.
They are preserved verbatim for reference only. **Do not submit them
from this repository** — use the cleaned scripts at the parent level
(`../../run_daint.sh` and `../../run_beverin.sh`), which wire in the
shared `common/activate.sh` + `common/setup_{daint,beverin}.sh`,
parameterize `REPS`/`CONFIGS`/`STAGE_SET`, and write outputs under
`results/{daint,beverin}/stage{4,8}/`.

## Known issues (intentional, not fixed)

- `run_full_permutations_stage4_daint.sh` and
  `run_full_permutations_stage4_beverin.sh` set `_STAGE=5` despite the
  filename. The original scripts compile against the stage-4 baseline
  but write output into `daint_full_permutations_5/` /
  `beverin_full_permutations_5/`. The cleaned scripts dispatch on
  `_STAGE` per iteration (`STAGE_SET="4 8"`), so this inconsistency is
  not inherited.
- SLURM `--output=` / `--error=` paths put logs in the submission cwd
  rather than under a structured `results/` tree.
- Spack and ROCm environment variables are re-exported inline instead
  of sourced from a shared env file.

Refer to these scripts only when you need the exact environment that
produced the paper results.
