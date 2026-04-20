# SC26 Layout Artifact — Experiments

Reproducibility root for the six experiments described in the Appendix
(Artifact Description). Each `EX_*/` folder is a self-contained
benchmark; `common/` holds the shared setup used by every experiment.

## Layout

| Folder | Paper element | Contributions |
|---|---|---|
| [E1_MatrixAdd/](E1_MatrixAdd/) | Fig. 4 | C₃, C₄ |
| [E2_Conjugation/](E2_Conjugation/) | Fig. 8 | C₂–C₄ |
| [E3_Transpose/](E3_Transpose/) | Fig. 9, Tab. III | C₂–C₄ |
| [E4_GAS/](E4_GAS/) | Fig. 10 | C₂–C₄ |
| [E5_USXX/](E5_USXX/) | Fig. 11, Lst. 1 | C₂–C₄ |
| [E6_VelocityTendencies/](E6_VelocityTendencies/) | Fig. 12–13, Tab. IV | C₃, C₄ |
| [Supplemental/](Supplemental/) | Fig. 2–3 (illustrations) | C₁ |

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

~17 hr per cluster end-to-end. Per-experiment breakdown in the AD
appendix.
