# SC26 Layout Artifact — Experiments

Reproducibility root. Each `EX_*/` is a self-contained benchmark;
`common/` holds the shared environment.

## Experiments

E0 is a one-shot NUMA STREAM-peak baseline (≈ 2 min). E1–E5 are
independent — dispatch up to five `sbatch` jobs in parallel. E6 is
the longest single experiment.

| Folder | Paper | Contributions | Runtime | Data prep | CSV path |
|---|---|---|---|---|---|
| [E0_NUMA/](E0_NUMA/) | Hardware § | baseline | ≈ 2 min | none | `results/{daint,beverin}/stream_peak_{cpu,gpu}.csv` |
| [E1_MatrixAdd/](E1_MatrixAdd/) | Fig. 4 | C₃, C₄ | ≈ 90 min | none | `results/{daint,beverin}/madd_{daint,beverin}_{cpu,gpu}.csv` |
| [E2_Conjugation/](E2_Conjugation/) | Fig. 8 | C₂–C₄ | ≈ 210 min | none | `results/{daint,beverin}/results_{cpu,gpu}_{inplace,oop}.csv` |
| [E3_Transpose/](E3_Transpose/) | Fig. 9, Tab. III | C₂–C₄ | ≈ 180 min | none | `results/{daint,beverin}/transpose_*.csv` + `transpose_metrics_{cpu,gpu}.csv` |
| [E4_GAS/](E4_GAS/) | Fig. 10 | C₂–C₄ | ≈ 60 min | ≈ 200 MB BaTiO₃ — `bash E4_GAS/download_data.sh` | `results/{daint,beverin}/zaxpy_sweep_{small,1gb}{,_cpu}.csv` |
| [E5_USXX/](E5_USXX/) | Fig. 11, Lst. 1 | C₂–C₄ | ≈ 60 min | ≈ 1 GB BaTiO₃ — `bash E5_USXX/download_data.sh` | `results/{daint,beverin}/addusxx_{cpu,gpu}_sweep.csv` |
| [E6_VelocityTendencies/](E6_VelocityTendencies/) | Fig. 12–13, Tab. IV | C₃, C₄ | ≈ 450 min | ICON R02B05 — per-subtask `download_data.sh` | per-subtask READMEs |
| [E8_LegacyVT/](E8_LegacyVT/) **(default)** | Fig. 14, Tab. V | C₃, C₄ | ≈ 240 min | ICON nproma=20480 — symlinked from E7 if present, else auto-fetched; reads `E6/access_analysis/{layout_candidates,winners}.json` (auto-regenerated on first run) | `{daint,beverin}_full_permutations_8/<config>_<shuffled\|unshuffled>.txt` |
| [E7_FullVelocityTendencies/](E7_FullVelocityTendencies/) **(WIP)** | (succeeds E8) | — | — | same as E8 | (WIP) |

E8 is the AD's full-velocity-tendencies path. E7 is an in-progress
SDFG-driven refactor of the same module; skip it for the AD. See the
top-level [README](../README.md#reviewer-quick-start) for the full
reviewer flow.

Total: ~22 hr per cluster end-to-end (E1–E5 parallel: ~10 hr; E6: ~7.5
hr; E8: ~4 hr; setup + analysis: ~25 min).

The proof-illustration figures (Figures 2–3) are pure matplotlib in
[`../Figures/`](../Figures/) — no SLURM. `bash ../Figures/plot_all.sh`
regenerates them.

The runtime figures (Figures 4, 8–14) have two sibling drivers under
[`../Figures/`](../Figures/):

| Driver | Reads | Writes |
|---|---|---|
| [`plot_paper_snapshot.sh`](../Figures/plot_paper_snapshot.sh) | `../PaperSnapshot/<exp>/results/` | `../Figures/GeneratedFigures/Runtime/<stem>.{pdf,png}` |
| [`plot_results.sh`](../Figures/plot_results.sh) | `<exp>/results/` (your local CSVs) | next to the CSVs |

The two destinations never overlap. Missing CSVs produce `[skip]` and
the sweep continues. Both pin DejaVu Sans via
`MATPLOTLIBRC=../Figures/matplotlibrc`.

## How to run any experiment

```bash
# 1. One-time per machine.
bash common/setup.sh

# 2. Submit.
cd EX_Name && sbatch run_{daint,beverin}.sh

# 3. Plot.
python plot_paper.py
```

`setup.sh` spack-loads CPython 3.13 + sqlite (arch auto-detect:
`python/asgm25z` on zen3, `python/6kewgi6` on neoverse_v2; override
with `SC26_PYTHON_SPEC`), creates `common/venv` (`--without-pip`,
bootstrapped via `get-pip.py` because spack's CPython lacks the
bundled wheel), clones DaCe `yakup/dev`, installs deps. Override the
DaCe branch with `DACE_BRANCH=...` when needed. **E8 (the default
full-velocity-tendencies path) requires `f2dace/staging`** for its
icon-artifacts/sc26_layout codegen pipeline; the run scripts switch
to it via `DACE_BRANCH=f2dace/staging` automatically. E7 (WIP) uses
`yakup/dev`; only relevant if you opt into the new pipeline.

Each `run_*.sh` sources `../common/activate.sh` (manual venv
activation; `bin/activate` doesn't exist on spack venvs) and
`../common/setup_{daint,beverin}.sh` (loads modules, sets pinning,
exports `CPU_CXX{,FLAGS,LDFLAGS}` / `GPU_CXX{,FLAGS,LDFLAGS}`). For
interactive runs, source the two manually before `bash run_*.sh`.

## Datasets

E0–E3 and the proof figures synthesize inputs in-process.
E4/E5/E6 (loopnest_1)/E7 auto-fetch their datasets on first `sbatch`
via the per-experiment `download_data.sh` (E7 uses
`tools/download_data.sh`); the guard is idempotent (`[[ -d <dir> ]] ||
bash download_data.sh`), so re-runs are free.

| Script | Dataset | Size |
|---|---|---|
| [`E4_GAS/download_data.sh`](E4_GAS/download_data.sh) | BaTiO₃ indirect-access index set | ≈ 200 MB |
| [`E5_USXX/download_data.sh`](E5_USXX/download_data.sh) | Serialized BaTiO₃ for QE addusxx_g | ≈ 1 GB |
| [`E6_VelocityTendencies/loopnest_1/download_data.sh`](E6_VelocityTendencies/loopnest_1/download_data.sh) | ICON R02B05 (shared by every loopnest_*) | ≈ 3 GB |
| [`E7_FullVelocityTendencies/tools/download_data.sh`](E7_FullVelocityTendencies/tools/download_data.sh) | ICON nproma=20480 | ≈ 9 GB |

Outbound HTTPS from ETH PolyBox; URL/checksum overridable via env
(`DATA_URL`, `EXPECTED_SHA256`).

## Hardware

- **Daint.Alps** — quad-GH200, 72-core Grace Neoverse V2 (512 GB/s
  LPDDR5X) + H200 (4 TB/s HBM3). STREAM Triad: CPU 1.807, GPU 3.780
  TB/s.
- **Beverin** — quad-MI300A, 24 Zen4 cores sharing 128 GB unified
  HBM3. STREAM Triad: CPU 1.161, GPU 4.294 TB/s.

Both need SLURM + exclusive single-node allocation.

## Software

- Spack CPython 3.13.8 (`python/asgm25z` zen3 / `python/6kewgi6`
  neoverse_v2); spack `sqlite` (the rest of stdlib's C-ext prereqs
  are RPATH-baked into the spack python).
- DaCe `yakup/dev` for everything; `f2dace/staging` only for E7's
  opt-in `tools/regenerate_baselines.sh`.
- Spack GCC 14, CUDA 12.9 (Daint; **must be < 13** — CUDA 13 removes
  fields DaCe's runtime probes and breaks E8 codegen) / ROCm 6.4.1 (Beverin), OpenBLAS
  0.3.29 / 0.3.30, cuTENSOR / hipTensor and HPTT (E3 only).
- pip deps: numpy scipy matplotlib pandas.

## Single-source CUDA/HIP pattern

Every GPU benchmark has a canonical `.cu` plus a 7-line `*_hip.cpp`
shim that `#include`s its `.cu` companion. `common/gpu_compat.cuh`
dispatches `gpu*` calls to `cuda*` under nvcc, `hip*` under hipcc.
**Edit the `.cu`, not the shim.**

## Reviewer hint — `# TODO: VERSION`

Version-sensitive pins are tagged `# TODO: VERSION` at the call site.
Likely overrides:

- [`../../setup.sh`](../../setup.sh) — `SPACK_*` short hashes;
  `spack find -L <pkg> target=zen3` to refresh.
- [`common/setup.sh`](common/setup.sh) /
  [`activate.sh`](common/activate.sh) — `SC26_PYTHON_SPEC`,
  `sqlite/<hash>`, `DACE_BRANCH`. All env-overridable.
- [`common/setup_{daint,beverin}.sh`](common/) — GCC / ROCm / CUDA /
  OpenBLAS specs.
- `EX_*/download_data.sh` — `DATA_URL` if upstream moves.
