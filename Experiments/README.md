# SC26 Layout Artifact — Experiments

Reproducibility root for the six experiments described in the Appendix
(Artifact Description). Each `EX_*/` folder is a self-contained
benchmark; `common/` holds the shared setup used by every experiment.

## Layout

E1–E5 are mutually independent — a reviewer can dispatch up to five
`sbatch` jobs in parallel. E6 is the longest single experiment; run it
last (or on a separate allocation). Jobs marked *download* require a
one-time data fetch before the first submission.

| Folder | Paper element | Contributions | Runtime (est.) | Data prep | CSV path |
|---|---|---|---|---|---|
| [E1_MatrixAdd/](E1_MatrixAdd/)     | Fig. 4              | C₃, C₄   | ≈ 90 min  | none                                         | `results/{daint,beverin}/madd_{daint,beverin}_{cpu,gpu}.csv` |
| [E2_Conjugation/](E2_Conjugation/) | Fig. 8              | C₂–C₄    | ≈ 210 min | none                                         | `results/{daint,beverin}/results_{cpu,gpu}_{inplace,oop}.csv` |
| [E3_Transpose/](E3_Transpose/)     | Fig. 9, Tab. III    | C₂–C₄    | ≈ 180 min | none                                         | `results/{daint,beverin}/transpose_*.csv` + `transpose_metrics_{cpu,gpu}.csv` |
| [E4_GAS/](E4_GAS/)                 | Fig. 10             | C₂–C₄    | ≈ 60 min  | ≈ 200 MB BaTiO₃ — `bash E4_GAS/download_data.sh` | `results/{daint,beverin}/zaxpy_sweep_{small,1gb}{,_cpu}.csv` |
| [E5_USXX/](E5_USXX/)               | Fig. 11, Lst. 1     | C₂–C₄    | ≈ 60 min  | ≈ 1 GB BaTiO₃ — `bash E5_USXX/download_data.sh`  | `results/{daint,beverin}/addusxx_{cpu,gpu}_sweep.csv` |
| [E6_VelocityTendencies/](E6_VelocityTendencies/) | Fig. 12–13, Tab. IV | C₃, C₄ | ≈ 690 min (≈ 150 min loopnest 1 + 5 × 60 min loopnests 2–6 + 240 min full module) | ICON R02B05 — per-subtask `download_data.sh` / `scripts/download_nproma20480_data.sh` | see per-subtask READMEs |
| [Supplemental/](Supplemental/)     | Fig. 2–3 (illustrations) | C₁ | < 5 min | none                                           | figures rendered directly                     |

## How to run any experiment

Every experiment uses the same three-step flow.

### 1. One-time global setup (run once per machine)

```bash
bash common/setup.sh
```

Installs Python 3.13 via `pyenv` (assumes `pyenv` itself is installed),
creates a venv at `common/venv`, clones DaCe into `common/dace`,
checks out branch `yakup/dev`, and `pip install -e`'s it with
plotting dependencies.

Override the DaCe branch when needed (E6 full module uses
`f2dace/staging`):

```bash
DACE_BRANCH=f2dace/staging bash common/setup.sh
```

### 2. Submit or run on a cluster

```bash
cd EX_Name
sbatch run_daint.sh       # Daint.Alps  (Grace CPU + Hopper GPU)
sbatch run_beverin.sh     # Beverin     (Zen4 CPU + MI300A APU)
```

Each `run_*.sh` internally sources:

1. `../common/activate.sh` — activates the pyenv venv and ensures DaCe
   is on the correct branch.
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

## Hardware

- **Daint.Alps** — quad-GH200 node, 72-core Grace Neoverse V2 CPU
  (512 GB/s LPDDR5X) + H200 GPU (4 TB/s HBM3). STREAM Triad peak:
  CPU 1.807 TB/s, GPU 3.780 TB/s.
- **Beverin** — quad-MI300A APU node, 24 Zen4 cores sharing 128 GB
  unified HBM3. STREAM Triad peak: CPU 1.161 TB/s, GPU 4.294 TB/s.

Both clusters access required: SLURM, exclusive single-node
allocations.

## Software

- pyenv (host-side; installed by the user)
- Python 3.13 (installed via `common/setup.sh`)
- DaCe branch `yakup/dev` for E1–E5 and E6/loopnest_1;
  branch `f2dace/staging` for E6/full_velocity_tendencies
- Spack-provided GCC 14, CUDA 12.9 (Daint) / ROCm 6.4.1 (Beverin),
  OpenBLAS 0.3.29 (Daint) / 0.3.30 (Beverin), cuTENSOR / hipTensor
  (E3 only), HPTT (E3 only)
- Python deps: numpy, scipy, matplotlib, pandas (installed by setup.sh)

## Expected time budget

~22 hr per cluster end-to-end: 90 + 210 + 180 + 60 + 60 min for E1–E5
(≈ 10 hr, parallelizable across five independent `sbatch` jobs) plus
≈ 11.5 hr for E6 (loopnest 1 ≈ 150 min, loopnests 2–6 ≈ 300 min,
full-module sweep ≈ 240 min), plus ≈ 25 min for one-time setup and
post-run analysis. Per-experiment and per-cluster breakdown is in the
AD appendix (Table "\arttime").
