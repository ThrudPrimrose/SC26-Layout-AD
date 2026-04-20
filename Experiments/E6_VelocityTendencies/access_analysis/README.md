# E6 / Access Analysis (Task T6.2)

Canonicalizes the 25 loop nests of `velocity_advection.f90` into
6 pattern classes and groups the 50 arrays into 7 layout-equivalent
groups. Emits candidate layout configurations consumed by
`loopnest_{1..6}/` and by `conflict_resolution/`.

## How to run

```bash
# 1. One-time (from repo root)
bash ../../common/setup.sh
source ../../common/activate.sh

# 2. Run the analysis
python analyze_access.py
python coaccess.py
python jaccard.py
```

No SLURM job required — this is Python-only, runs on any node.

## Files

- `velocity_advection.f90` — original ICON source.
- `velocity_advection_raw.f90` — verbatim preprocessed input.
- `velocity_advection_preprocessed.f90` — annotated variant consumed
  by the analyzer.
- `velocity_advection_preprocessed.analysis.md` — reference output
  of the analysis.
- `omp_definitions.inc` — OpenMP macro definitions required to
  preprocess the Fortran source.
- `analyze_access.py` / `coaccess.py` / `jaccard.py` — analysis
  scripts.
- `analysis.md` — human-readable writeup of the pattern classes.

## Outputs

- `layout_candidates.json` — per-group candidate layouts scored by µ/Δ.
  Consumed by `../loopnest_{1..6}/`.
- `analysis.md` — narrative summary.
