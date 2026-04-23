# SC26 Layout Artifact — Experiments

Reproducibility root for the six experiments described in the Appendix
(Artifact Description). Each `EX_*/` folder is a self-contained
benchmark; `common/` holds the shared setup used by every experiment.

## Layout

E0 is a one-shot NUMA-aware STREAM peak baseline (≈ 2 min). E1–E5 are
mutually independent — a reviewer can dispatch up to five `sbatch` jobs
in parallel. E6 is the longest single experiment; run it last (or on a
separate allocation). Jobs marked *download* require a one-time data
fetch before the first submission.

| Folder | Paper element | Contributions | Runtime (est.) | Data prep | CSV path |
|---|---|---|---|---|---|
| [E0_NUMA/](E0_NUMA/)               | Hardware § table    | baseline | ≈ 2 min   | none                                         | `results/{daint,beverin}/stream_peak_{cpu,gpu}.csv`       |
| [E1_MatrixAdd/](E1_MatrixAdd/)     | Fig. 4              | C₃, C₄   | ≈ 90 min  | none                                         | `results/{daint,beverin}/madd_{daint,beverin}_{cpu,gpu}.csv` |
| [E2_Conjugation/](E2_Conjugation/) | Fig. 8              | C₂–C₄    | ≈ 210 min | none                                         | `results/{daint,beverin}/results_{cpu,gpu}_{inplace,oop}.csv` |
| [E3_Transpose/](E3_Transpose/)     | Fig. 9, Tab. III    | C₂–C₄    | ≈ 180 min | none                                         | `results/{daint,beverin}/transpose_*.csv` + `transpose_metrics_{cpu,gpu}.csv` |
| [E4_GAS/](E4_GAS/)                 | Fig. 10             | C₂–C₄    | ≈ 60 min  | ≈ 200 MB BaTiO₃ — `bash E4_GAS/download_data.sh` | `results/{daint,beverin}/zaxpy_sweep_{small,1gb}{,_cpu}.csv` |
| [E5_USXX/](E5_USXX/)               | Fig. 11, Lst. 1     | C₂–C₄    | ≈ 60 min  | ≈ 1 GB BaTiO₃ — `bash E5_USXX/download_data.sh`  | `results/{daint,beverin}/addusxx_{cpu,gpu}_sweep.csv` |
| [E6_VelocityTendencies/](E6_VelocityTendencies/) | Fig. 12–13, Tab. IV | C₃, C₄ | ≈ 690 min (≈ 150 min loopnest 1 + 5 × 60 min loopnests 2–6 + 240 min full module) | ICON R02B05 — per-subtask `download_data.sh` / `scripts/download_nproma20480_data.sh` | see per-subtask READMEs |
| [NumaStream/](NumaStream/)         | supplemental NUMA sweep  | baseline | ≈ 5 min | none                                        | `results/{daint,beverin}/numa_{cpu,gpu}.csv`  |

The proof-illustration figures (Figures 2–3 in the main text) live in
[`../Figures/`](../Figures/) — pure matplotlib scripts, no benchmark,
and no submission to SLURM. Regenerate with `bash ../Figures/plot_all.sh`.

## How to run any experiment

Every experiment uses the same three-step flow.

### 1. One-time global setup (run once per machine)

```bash
bash common/setup.sh
```

`spack load`s a pinned CPython + `sqlite` (arch auto-detect:
`python/asgm25z` on x86_64 / zen3, `python/6kewgi6` on aarch64 /
neoverse_v2). Readline, bz2, lzma, ctypes, ssl, zlib all come baked
into the spack python via RPATH, so only `sqlite` is `spack load`ed
externally. The script then creates a venv at `common/venv` using that
python (with a pip bootstrap workaround — see note below), clones
DaCe into `common/dace` on branch `yakup/dev`, `pip install -e`'s it,
and pip-installs numpy / scipy / matplotlib / pandas.

Override the DaCe branch when needed (E6 full module uses
`f2dace/staging`):

```bash
DACE_BRANCH=f2dace/staging bash common/setup.sh
```

Override the python spec to pin a specific hash:

```bash
SC26_PYTHON_SPEC=python/6kewgi6 bash common/setup.sh   # force aarch64 hash
```

> **Venv caveats on spack CPython** — spack's CPython build ships with
> empty `venv/scripts/{common,posix}` template dirs and a missing pip
> wheel in `ensurepip/_bundled/`. `setup.sh` works around both:
> creates the venv with `--without-pip`, bootstraps pip via
> `get-pip.py`, and activates the venv by exporting `VIRTUAL_ENV` +
> `PATH` directly (no `bin/activate` exists). `activate.sh` does the
> same manual activation. Do not try to `source venv/bin/activate` —
> it doesn't exist.

### 2. Submit or run on a cluster

```bash
cd EX_Name
sbatch run_daint.sh       # Daint.Alps  (Grace CPU + Hopper GPU)
sbatch run_beverin.sh     # Beverin     (Zen4 CPU + MI300A APU)
```

Each `run_*.sh` internally sources:

1. `../common/activate.sh` — re-loads the spack python, activates the
   `common/venv`, and ensures DaCe is on the correct branch.
2. `../common/setup_daint.sh` *or* `../common/setup_beverin.sh` —
   loads platform modules (spack), sets OMP/SLURM pinning, and
   exports `CPU_CXX`, `CPU_CXXFLAGS`, `CPU_LDFLAGS`, `GPU_CXX`,
   `GPU_CXXFLAGS`, `GPU_LDFLAGS`.

The run script then builds CPU + GPU binaries using those flag
variables and writes CSV results to `results/{daint,beverin}/`.

For interactive debugging without SLURM:

```bash
source common/activate.sh
source common/setup_daint.sh       # or setup_beverin.sh
cd EX_Name && bash run_daint.sh
```

### 3. Plot

```bash
cd EX_Name
python plot_paper.py
```

Reads `results/{daint,beverin}/*.csv` and writes the paper figure(s).

## Data-loading scripts and what they assume

Every data download is a one-time step gated behind an explicit
`bash download_data.sh`. No experiment fetches data at `sbatch` time.

| Script | Fetches | Size | Assumes |
|---|---|---|---|
| [`E4_GAS/download_data.sh`](E4_GAS/download_data.sh) | BaTiO₃ indirect-access index set for `zaxpy` | ≈ 200 MB | Outbound HTTPS (ETH PolyBox); run from inside `E4_GAS/`; idempotent (skips if already present). |
| [`E5_USXX/download_data.sh`](E5_USXX/download_data.sh) | Serialized BaTiO₃ state for QE `addusxx_g` | ≈ 1 GB | Outbound HTTPS (ETH PolyBox); run from inside `E5_USXX/`; writes to `bin/`. |
| [`E6_VelocityTendencies/loopnest_1/download_data.sh`](E6_VelocityTendencies/loopnest_1/download_data.sh) | ICON R02B05 velocity-tendencies serialized input | ≈ 3 GB | Outbound HTTPS; required for every `loopnest_{1..6}` subtask. |
| [`E6_VelocityTendencies/full_velocity_tendencies/scripts/download_nproma20480_data.sh`](E6_VelocityTendencies/full_velocity_tendencies/scripts/download_nproma20480_data.sh) | ICON full-module `nproma=20480` dataset | ≈ 8 GB | Outbound HTTPS; required for stage4/stage8 full-module sweeps. |

E0, E1, E2, E3, and the `Figures/` proof illustrations all synthesize
their inputs in-process (no network access required).

Setup / environment-loading scripts and their assumptions:

| Script | Loads | Assumes |
|---|---|---|
| [`common/setup.sh`](common/setup.sh) | spack python + sqlite; venv at `common/venv` (pip bootstrapped via get-pip.py); DaCe clone + editable install; plotting deps | `spack` on PATH; short-hash specs resolve for the current arch (`uname -m` auto-select); outbound HTTPS to pypi + github. |
| [`common/activate.sh`](common/activate.sh) | spack python + sqlite; manual venv activation (exports `VIRTUAL_ENV` + `PATH`); DaCe branch check | `setup.sh` has been run; `DACE_BRANCH` defaults to `yakup/dev`; `SC26_PYTHON_SPEC` arch-auto-detects (override to force). |
| [`common/setup_daint.sh`](common/setup_daint.sh) | GCC 14, CUDA 12.9, OpenBLAS 0.3.29, OpenMP / SLURM pinning for Grace | Daint.Alps (linux-sles15-neoverse_v2); spack env with those specs. |
| [`common/setup_beverin.sh`](common/setup_beverin.sh) | GCC 14, ROCm 6.x, OpenBLAS 0.3.30, OpenMP / SLURM pinning for Zen4 | Beverin (linux-sles15-zen3); spack env with those specs; ROCm at `/opt/rocm`. |

## Reviewer hint — `# TODO: VERSION`

Version-sensitive knobs in this artifact are marked with `# TODO: VERSION`
in the scripts. A reviewer may need to adjust:

- [`../../setup.sh`](../../setup.sh) — pinned `SPACK_*` short hashes
  (`python/asgm25z`, `readline/sg63hivx`, …). Regenerate with
  `spack find -L <pkg> target=zen3` if the spack env is re-concretized.
- [`common/setup.sh`](common/setup.sh) / [`activate.sh`](common/activate.sh)
  — `SC26_PYTHON_SPEC` (auto: `python/asgm25z` zen3 / `python/6kewgi6`
  neoverse_v2), `sqlite` short hash per arch, `DACE_BRANCH=yakup/dev`.
  All overridable via env vars.
- [`common/setup_{daint,beverin}.sh`](common/) — GCC / ROCm / CUDA /
  OpenBLAS spack specs.
- `EX_*/download_data.sh` — dataset URL; if the upstream mirror moves,
  edit the URL at the top of the script.

## Hardware

- **Daint.Alps** — quad-GH200 node, 72-core Grace Neoverse V2 CPU
  (512 GB/s LPDDR5X) + H200 GPU (4 TB/s HBM3). STREAM Triad peak:
  CPU 1.807 TB/s, GPU 3.780 TB/s.
- **Beverin** — quad-MI300A APU node, 24 Zen4 cores sharing 128 GB
  unified HBM3. STREAM Triad peak: CPU 1.161 TB/s, GPU 4.294 TB/s.

Both clusters access required: SLURM, exclusive single-node
allocations.

## Software

- Spack-provided CPython 3.13.8 on zen3 (`python/asgm25z`) /
  neoverse_v2 (`python/6kewgi6`); venv at `common/venv` (pip
  bootstrapped via get-pip.py; manual activation).
- DaCe branch `yakup/dev` for E0–E5 and E6/loopnest_1;
  branch `f2dace/staging` for E6/full_velocity_tendencies
- Spack-provided GCC 14, CUDA 12.9 (Daint) / ROCm 6.4.1 (Beverin),
  OpenBLAS 0.3.29 (Daint) / 0.3.30 (Beverin), cuTENSOR / hipTensor
  (E3 only), HPTT (E3 only)
- Spack-provided `sqlite` (other stdlib C-ext prereqs — readline,
  bz2, lzma, ctypes, ssl, zlib — are baked into the spack python
  via RPATH, not loaded separately).
- Python deps: numpy, scipy, matplotlib, pandas (installed by setup.sh)

## Expected time budget

~22 hr per cluster end-to-end: ≈ 2 min for E0 (NUMA baseline) + 90 +
210 + 180 + 60 + 60 min for E1–E5
(≈ 10 hr, parallelizable across five independent `sbatch` jobs) plus
≈ 11.5 hr for E6 (loopnest 1 ≈ 150 min, loopnests 2–6 ≈ 300 min,
full-module sweep ≈ 240 min), plus ≈ 25 min for one-time setup and
post-run analysis. Per-experiment and per-cluster breakdown is in the
AD appendix (Table "\arttime").
