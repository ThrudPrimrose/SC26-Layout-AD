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

## Reviewer quick-start

End-to-end reproduction in five steps; ~22 hr per cluster.

1. Clone this repo and `bash Experiments/common/setup.sh` (one-time per
   machine — spack-loads Python 3.13, creates `common/venv`, clones
   DaCe).
2. Submit microbenchmarks: `cd Experiments/E1_MatrixAdd && sbatch run_daint.sh`
   (or `run_beverin.sh`); repeat for E2–E6 in any order.
3. Submit the full velocity-tendencies sweep:
   `cd Experiments/E8_LegacyVT && sbatch run_daint.sh` (default
   `CONFIGS="winner_v1,winner_v2,winner_v6"`).
4. Plot everything: `bash Figures/plot_all.sh Runtime`. Figures land in
   `Figures/GeneratedFigures/Runtime/`.
5. Cross-check against the per-experiment "Expected result" anchors in
   each `Experiments/EX_*/README.md`.

Per-experiment READMEs under `Experiments/EX_*/README.md` document the
exact CSV filenames each run script emits and the expected qualitative
outcome.

## Calibration update

The submitted paper's MI300A %-of-STREAM annotations were normalized
against ~4.3 TB/s; between submission and AD freeze the cache-flush
kernel was rewritten as a 3-sweep 8192² Jacobi with buffer swap to
defeat dead-code-elimination by the GPU compiler, and the MI300A STREAM
Triad peak was independently cross-checked with AMD performance
engineering at ~3.55 TB/s. All E1–E6 and E8 figures in this artifact
are normalized to the corrected peak. Qualitative trends — relative
ordering of layouts, gap between unpermuted and the V_k winners —
continue to hold.

## Figures

Three drivers, separate output destinations so they never clash:

| Script | Reads | Writes |
|---|---|---|
| `Figures/plot_paper_snapshot.sh` | `PaperSnapshot/<exp>/results/*.csv` | `Figures/GeneratedFigures/Runtime/<stem>.{pdf,png}` |
| `Figures/plot_results.sh` | `Experiments/<exp>/results/*.csv` | next to the CSVs |
| `Figures/plot_all.sh` | (umbrella) | illustrative groups + `Peaks` + both above |

Both runtime drivers iterate over E1–E5,
`E6_VelocityTendencies/loopnest_{1..6}`, and E8 (the legacy full
velocity-tendencies pipeline that's the AD default for §IV-D / Fig 14;
E7 is included as work-in-progress and is plotted only when its CSVs
exist). Missing CSVs produce a one-line `[skip]` and the sweep
continues. All three pin DejaVu Sans via
`MATPLOTLIBRC=Figures/matplotlibrc` to avoid STIX/CM warnings on
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
    ├── E7_FullVelocityTendencies/  WIP. New SDFG-driven pipeline (DaCe
    │                               yakup/dev + OffloadVelocityToGPU +
    │                               PermuteDimensions). Kept in the tree
    │                               for the next iteration; not the AD's
    │                               default reproduction path -- E8 is.
    └── E8_LegacyVT/                Figure 14, Table V (DEFAULT). Legacy
                                    icon-artifacts/sc26_layout pipeline
                                    on f2dace/staging. Drives
                                    run_stage8_permutations.py with
                                    --configs winner_v{1,2,6}; reads
                                    E6/access_analysis/{layout_candidates,
                                    winners}.json (auto-regenerated).
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

## Canonical compile flags

All GCC / nvcc / hipcc compilations in this repo use a single canonical
flag set, centralized in `Experiments/common/setup_{daint,beverin}.sh`
as `CPU_CXXFLAGS` / `GPU_CXXFLAGS`.

- **GCC (CPU, both platforms)**:
  `-O3 -ffast-math -fno-trapping-math -fno-math-errno -march=native -mtune=native -fno-vect-cost-model -fopenmp -std=c++17`
- **nvcc (Daint GPU)**: device-side `-O3 --use_fast_math`; host-side
  GCC pass receives the full CPU set via `-Xcompiler=...`.
- **hipcc (Beverin GPU)**: same set as GCC except no
  `-fno-vect-cost-model` (Clang does not implement it — accepted
  exception), plus HIP-specific `-munsafe-fp-atomics` /
  `-mllvm -amdgpu-early-inline-all=true`.

`-fno-trapping-math` and `-fno-math-errno` are already implied by
`-ffast-math`; they are listed explicitly so intent survives any
future `-ffast-math` definition drift. `-fno-vect-cost-model` forces
GCC's loop vectorizer to ignore its cost model (Clang has no
equivalent; rely on `-O3`'s vectorizer there).

## OpenMP scheduling policy

Every OpenMP loop in the repo uses `schedule(static)`, and both platform
setup scripts export `OMP_SCHEDULE=static`. No `schedule(dynamic)` /
`schedule(guided)` / `schedule(runtime)` / `schedule(auto)` anywhere in
our sources — dynamic scheduling introduces jitter that can mask or
exaggerate bandwidth effects and defeats reproducibility. CPU pinning
is handled via `SLURM_CPU_BIND=cores` (set in the setup scripts) rather
than `#SBATCH --cpu-bind` flags.

## Utilities

- [`Experiments/clean_core_dumps.sh`](Experiments/clean_core_dumps.sh)
  — recursively removes `core_nid*` core dumps from the Experiments
  tree. Dry-run by default; pass `--delete [-y]` to actually remove
  them. Cleans up after any benchmark crash on Alps / Daint.

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
