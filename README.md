# SC26-Layout-AD

Reproducibility artifact for the SC26 paper **"Layout Algebra and Cost
Metrics for Memory-Hierarchy-Aware Array Rearrangement"**. This repo
contains everything a reviewer needs to reproduce Figures 4 and 8–13
and the analytic cost-metric tables.

## Where to start

1. **[`latex/`](latex/)** — the Artifact Description / Artifact
   Evaluation appendix (`sc26_ad_ae_template.tex`). Build with
   `make -C latex` to get `sc26_ad_ae_template.pdf`. Read this first
   for contribution / experiment / time-budget context.

2. **[`Experiments/README.md`](Experiments/README.md)** — the entry
   point for running any experiment. Has the cross-reference table
   (experiment → paper figure → runtime → data prep → CSV path),
   the one-time `common/setup.sh` recipe, and the per-cluster
   activation flow.

3. **[`Experiments/common/`](Experiments/common/)** — shared
   infrastructure every experiment depends on:
   - `setup.sh`, `activate.sh`, `setup_{daint,beverin}.sh` — environment
   - `jacobi_flush.h` — canonical 8192² Jacobi cache flush (shared)
   - `numa_util.h` — NUMA-aware allocator + first-touch
   - `prng.h` — deterministic Xor64 PRNG (canonical seed `SC26_SEED=42`)
   - `plot_util.py` — repo-wide plotting policy (no outlier trimming,
     Scott-bandwidth KDE, 200 evaluation points)

## Quick start (10-minute first experiment)

```bash
# 1. One-time per machine (installs pyenv Python 3.13, DaCe yakup/dev, deps)
bash Experiments/common/setup.sh

# 2. Pick a short experiment and submit
cd Experiments/E1_MatrixAdd
sbatch run_daint.sh        # or run_beverin.sh

# 3. Plot
python plot_paper.py
```

The per-experiment READMEs under `Experiments/EX_*/README.md` give
the analogous recipe for each experiment and the exact CSV filenames
that the run scripts emit.

## Repo structure

```
SC26-Layout-AD/
├── README.md                 (this file)
├── latex/                    (AD / AE appendix sources + built PDF)
├── AD/                       (legacy template workspace; latex/ is canonical)
└── Experiments/
    ├── README.md             (experiment entry point + runtime table)
    ├── common/               (shared env + shared headers + plot util)
    ├── E1_MatrixAdd/         (Figure 4)
    ├── E2_Conjugation/       (Figure 8)
    ├── E3_Transpose/         (Figure 9, Table III)
    ├── E4_GAS/               (Figure 10; BaTiO₃ indirect-access data)
    ├── E5_USXX/              (Figure 11, Listing 1; QE addusxx_g kernel)
    ├── E6_VelocityTendencies/(Figures 12–13, Table IV; ICON dycore)
    │   ├── access_analysis/
    │   ├── loopnest_{1..6}/
    │   ├── conflict_resolution/
    │   └── full_velocity_tendencies/
    └── Supplemental/         (illustrative figures for the proof, Fig. 2–3)
```

## Hardware targets

- **Daint.Alps** — quad-GH200 node, Grace Neoverse V2 + H200 GPU.
- **Beverin** — quad-MI300A APU node, Zen 4 + MI300A integrated GPU.

Both require SLURM + exclusive single-node allocation. Results should
generalize to any cluster with comparable cache-line sizes
(64 B CPU, 128 B GPU); see
[`Experiments/README.md`](Experiments/README.md) for details.

## Determinism and statistical methodology

- 100 timed repetitions after 5 warm-ups, for every configuration.
- Canonical cache flush between reps: 3-sweep 8192² Jacobi stencil
  with buffer swap (`common/jacobi_flush.h`).
- All random draws seeded from `SC26_SEED = 42` via a shared XOR-shift
  PRNG (`common/prng.h`).
- Plots show every collected sample — no outlier trimming.
- Violin-plot KDE uses Scott's rule + 200 evaluation points,
  identical across every figure.

See the Statistical Methodology block in
`latex/sc26_ad_ae_template.tex` for the full description.
