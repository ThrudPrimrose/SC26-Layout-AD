# SC26-Layout-AD

Reproducibility artifact for the SC26 paper **"Layout Algebra and Cost
Metrics for Memory-Hierarchy-Aware Array Rearrangement"**: everything
needed to reproduce Figures 4 and 8–14 and the analytic cost-metric
tables.

## Where to start

1. **[`Latex/sc26_ad_ae_template.tex`](Latex/)** — AD/AE appendix.
   `make -C Latex` builds the PDF. Read first for contributions,
   experiment narrative, and time budget.
2. **[`Experiments/README.md`](Experiments/README.md)** — entry point
   for running any experiment: cross-reference table, one-time setup,
   per-cluster activation.
3. **[`Experiments/common/`](Experiments/common/)** — shared infra:
   env scripts, `jacobi_flush.h`, `numa_util.h`, `prng.h`,
   `plot_util.py`.
4. **[`Figures/`](Figures/)** — figure regeneration drivers (see
   below).
5. **[`PaperSnapshot/`](PaperSnapshot/)** — frozen paper-canonical
   CSVs. Mirrors `Experiments/<exp>/results/` so the same
   `plot_paper.py` reads from either root unchanged.

## Quick start

```bash
# 1. One-time per machine: spack-loads Python 3.13 + sqlite, creates
#    common/venv, clones DaCe yakup/dev, installs deps.
#    Arch auto-detect: zen3 -> python/asgm25z, neoverse_v2 ->
#    python/6kewgi6 (override with SC26_PYTHON_SPEC).
bash Experiments/common/setup.sh

# 2. Pick an experiment and submit
cd Experiments/E1_MatrixAdd && sbatch run_daint.sh   # or run_beverin.sh

# 3. Plot — figures land next to the CSVs in results/{daint,beverin}/
python plot_paper.py
```

Per-experiment READMEs under `Experiments/EX_*/README.md` document the
exact CSV filenames each run script emits.

## Figures

Three drivers, separate output destinations so they never clash:

| Script | Reads | Writes |
|---|---|---|
| `Figures/plot_paper_snapshot.sh` | `PaperSnapshot/<exp>/results/*.csv` | `Figures/GeneratedFigures/Runtime/<stem>.{pdf,png}` |
| `Figures/plot_results.sh` | `Experiments/<exp>/results/*.csv` | next to the CSVs |
| `Figures/plot_all.sh` | (umbrella) | illustrative groups + `Peaks` + both above |

Both runtime drivers iterate over E1–E5,
`E6_VelocityTendencies/loopnest_{1..6}`, and E7. Missing CSVs produce a
one-line `[skip]` and the sweep continues. All three pin DejaVu Sans
via `MATPLOTLIBRC=Figures/matplotlibrc` to avoid STIX/CM warnings on
stock environments.

`plot_all.sh` accepts subset names: `PaperSnapshot`, `Results`,
`Runtime`, `Peaks`, or any illustrative group (`AccessCost`,
`Pebble_Game`, `LayoutTransformations`, `Replay`).

Prerequisites: `matplotlib numpy pandas scipy`. On a cluster, `source
Experiments/common/activate.sh` first.

## Repo structure

```
SC26-Layout-AD/
├── Latex/                    AD/AE appendix sources + built PDF
├── Figures/                  regeneration drivers + illustrative scripts
│   └── GeneratedFigures/Runtime/   paper-canonical runtime PDFs (from PaperSnapshot)
├── PaperSnapshot/            frozen paper-canonical CSVs
└── Experiments/
    ├── common/               shared env + headers + plot util
    ├── E0_NUMA/              NUMA-aware STREAM peak baseline
    ├── E1_MatrixAdd/         Figure 4
    ├── E2_Conjugation/       Figure 8
    ├── E3_Transpose/         Figure 9, Table III
    ├── E4_GAS/               Figure 10; BaTiO₃ indirect-access data
    ├── E5_USXX/              Figure 11, Listing 1; QE addusxx_g
    ├── E6_VelocityTendencies/  Figures 12–13, Table IV (per-loopnest)
    │   ├── access_analysis/
    │   ├── loopnest_{1..6}/
    │   └── conflict_resolution/
    └── E7_FullVelocityTendencies/  Figure 14, Table V (full-module GPU
                                    permutation sweep on shipped stage 5
                                    SDFGs; reads
                                    E6/access_analysis/layout_candidates.json
                                    — run E6 T6.2 first; opt-in
                                    tools/regenerate_baselines.sh rebuilds
                                    stage 5 from F90 on f2dace/staging)
```

STREAM peaks for bandwidth normalization live in
[`Experiments/common/stream_peak.json`](Experiments/common/stream_peak.json);
`E0_NUMA/plot_paper.py` overwrites from measured CSVs and every other
`plot_paper.py` reads via `plot_util.load_stream_peaks()`.

## Hardware

- **Daint.Alps** — quad-GH200 node (Grace Neoverse V2 + H200).
- **Beverin** — quad-MI300A APU node (Zen 4 + MI300A integrated GPU).

Both need SLURM + exclusive single-node allocation. Results should
generalize to any cluster with comparable cache-line sizes (64 B CPU,
128 B GPU).

## Statistical methodology

- 100 timed reps after 5 warm-ups, every configuration.
- Cache flush between reps: 3-sweep 8192² Jacobi
  (`common/jacobi_flush.h`).
- All RNG seeded from `SC26_SEED = 42` (`common/prng.h`).
- No outlier trimming. Violin-plot KDE: Scott's rule, 200 eval points,
  identical across every figure.

Full description in `Latex/sc26_ad_ae_template.tex`.

## Reviewer hint — `# TODO: VERSION`

Version-sensitive pins are tagged `# TODO: VERSION` at the call site.
Places to check first:

- `../setup.sh` — `SPACK_*` short hashes.
- [`Experiments/common/setup.sh`](Experiments/common/setup.sh) +
  [`activate.sh`](Experiments/common/activate.sh) — `SC26_PYTHON_SPEC`
  (zen3 → `python/asgm25z`, neoverse_v2 → `python/6kewgi6`),
  `sqlite/<hash>`, `DACE_BRANCH=yakup/dev`.
- [`Experiments/common/setup_{daint,beverin}.sh`](Experiments/common/) —
  GCC / ROCm / CUDA / OpenBLAS specs.
- `Experiments/E{4,5,6}_*/download_data.sh` — upstream dataset URLs.

All scripts respect env-var overrides (`SC26_PYTHON_SPEC=...`,
`DACE_BRANCH=...`, `DATA_URL=...`).
