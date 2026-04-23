# SC26-Layout-AD

Reproducibility artifact for the SC26 paper **"Layout Algebra and Cost
Metrics for Memory-Hierarchy-Aware Array Rearrangement"**. This repo
contains everything a reviewer needs to reproduce Figures 4 and 8–13
and the analytic cost-metric tables.

## Where to start

1. **[`Latex/`](Latex/)** — the Artifact Description / Artifact
   Evaluation appendix (`sc26_ad_ae_template.tex`). Build with
   `make -C Latex` to get `sc26_ad_ae_template.pdf`. Read this first
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

4. **[`Figures/`](Figures/)** — single regeneration driver for **every**
   figure in the paper. `bash Figures/plot_all.sh` rebuilds the
   illustrative proof figures (2–3, supplemental) AND the runtime
   figures (4, 8–11) from both the frozen paper snapshot and any fresh
   local reproduction, all in one command. Outputs land under
   [`Figures/GeneratedFigures/`](Figures/GeneratedFigures/). See
   [Regenerate all figures](#regenerate-all-figures) below for the
   subset flags (`Runtime`, `Peaks`, etc.).

5. **[`PaperSnapshot/`](PaperSnapshot/)** — frozen, paper-canonical CSVs
   that produced the submitted figures. Mirrors `Experiments/<exp>/results/`
   layout so every `plot_paper.py` reads from either root unchanged;
   `plot_all.sh` plots both sides side-by-side.

## Quick start (10-minute first experiment)

```bash
# 1. One-time per machine. Spack-loads Python 3.13 + sqlite, creates
#    common/venv (with a pip bootstrap workaround — see setup.sh
#    header), clones DaCe yakup/dev, installs deps. Arch auto-detect:
#    x86_64 (beverin, zen3)  -> python/asgm25z
#    aarch64 (daint, nv_v2)  -> python/6kewgi6
#    Override with SC26_PYTHON_SPEC=python/<hash> if needed.
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

## Regenerate all figures

```bash
bash Figures/plot_all.sh                  # everything — all groups
bash Figures/plot_all.sh Runtime          # only Fig. 4, 8–11 (paper + new)
bash Figures/plot_all.sh Peaks            # refresh common/stream_peak.json
bash Figures/plot_all.sh AccessCost       # one illustrative group
bash Figures/plot_all.sh Peaks Runtime    # any combination
```

The `Runtime` group runs every `Experiments/<exp>/plot_paper.py` twice
in one invocation:

- from `PaperSnapshot/<exp>/` (frozen, paper-canonical CSVs) →
  `Figures/GeneratedFigures/Runtime/`
- from `Experiments/<exp>/` (fresh local reproduction after an `sbatch`
  run) → `Figures/GeneratedFigures/Runtime/new/`

Both runs land flat in their respective folders — no filename overlap
because the destinations differ — so reviewers can `diff` / image-diff
the two folders to check their rerun matches the paper. Missing CSVs
(e.g. an experiment not yet re-run locally, or an empty snapshot slot)
produce a `[warn]` line and are skipped for that step, never aborting
the rest of the sweep.

Illustrative-group outputs (`AccessCost`, `Pebble_Game`,
`LayoutTransformations`, `Replay`) land under
`Figures/GeneratedFigures/<group>/` unchanged. `Peaks` refreshes
`Experiments/common/stream_peak.json` from measured CSVs (no figures).

Prerequisites: `matplotlib`, `numpy`, `pandas`, `scipy`. On a cluster,
`source Experiments/common/activate.sh` first; elsewhere, a plain `pip
install matplotlib numpy pandas scipy` is enough.

## Repo structure

```
SC26-Layout-AD/
├── README.md                 (this file)
├── Latex/                    (AD/AE appendix sources + built PDF)
├── Figures/                  (figure regeneration driver + illustrative scripts)
│   ├── plot_all.sh           (ONE entry point: illustrative + runtime + peaks)
│   ├── AccessCost/           (block / NUMA access-cost plots)
│   ├── Pebble_Game/          (pebble-game illustrations)
│   ├── LayoutTransformations/(stage-by-stage layout transform figures)
│   ├── Replay/               (stride replay figure)
│   └── GeneratedFigures/     (all .pdf / .png outputs, one folder per group)
│       ├── Runtime/          (Fig. 4, 8–11 from PaperSnapshot CSVs)
│       └── Runtime/new/      (Fig. 4, 8–11 from Experiments/*/results/)
├── PaperSnapshot/            (frozen paper-canonical CSVs; mirrors results/ layout)
└── Experiments/
    ├── README.md             (experiment entry point + runtime table)
    ├── common/               (shared env + shared headers + plot util)
    ├── E0_NUMA/              (NUMA-aware STREAM peak baseline; CPU + GPU)
    ├── E1_MatrixAdd/         (Figure 4)
    ├── E2_Conjugation/       (Figure 8)
    ├── E3_Transpose/         (Figure 9, Table III)
    ├── E4_GAS/               (Figure 10; BaTiO₃ indirect-access data)
    ├── E5_USXX/              (Figure 11, Listing 1; QE addusxx_g kernel)
    └── E6_VelocityTendencies/(Figures 12–13, Table IV; ICON dycore)
        ├── access_analysis/
        ├── loopnest_{1..6}/
        ├── conflict_resolution/
        └── full_velocity_tendencies/
```

STREAM peak values for all per-experiment bandwidth normalizations live in
[`Experiments/common/stream_peak.json`](Experiments/common/stream_peak.json),
which `E0_NUMA/plot_paper.py` overwrites from measured CSVs when available
and every other `plot_paper.py` reads via `plot_util.load_stream_peaks()`.

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
`Latex/sc26_ad_ae_template.tex` for the full description.

## Reviewer hint — `# TODO: VERSION`

Every version-sensitive pin in this artifact is tagged `# TODO: VERSION`
in the corresponding script. Places to check / override first:

- `../setup.sh` — `SPACK_*` short hashes (python, readline, sqlite, …).
- [`Experiments/common/setup.sh`](Experiments/common/setup.sh) +
  [`activate.sh`](Experiments/common/activate.sh) — arch-pinned
  `SC26_PYTHON_SPEC` (`python/asgm25z` on zen3, `python/6kewgi6` on
  neoverse_v2) and `sqlite/<hash>`. `DACE_BRANCH=yakup/dev`.
- [`Experiments/common/setup_{daint,beverin}.sh`](Experiments/common/) —
  GCC / ROCm / CUDA / OpenBLAS spack specs.
- `Experiments/E{4,5,6}_*/download_data.sh` — upstream dataset URL.

All scripts respect env-var overrides (`SC26_PYTHON_SPEC=...`,
`DACE_BRANCH=...`, `DATA_URL=...`) where applicable. See
[`Experiments/README.md`](Experiments/README.md) for the full table.

> **Note on venv activation** — the spack-built CPython ships without
> venv's script templates and without a bundled pip wheel, so
> `common/setup.sh` creates the venv with `--without-pip`, bootstraps
> pip via `get-pip.py`, and `common/activate.sh` activates it by
> exporting `VIRTUAL_ENV` + `PATH` manually instead of `source
> venv/bin/activate` (that file does not exist).
