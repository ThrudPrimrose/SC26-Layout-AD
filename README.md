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

4. **[`Figures/`](Figures/)** — regeneration drivers for every figure.
   Three scripts:
   - [`Figures/plot_paper_snapshot.sh`](Figures/plot_paper_snapshot.sh) —
     rebuilds the **paper-canonical** runtime figures (4, 8–11) from
     the frozen CSVs under [`PaperSnapshot/`](PaperSnapshot/); outputs
     land in [`Figures/GeneratedFigures/Runtime/`](Figures/GeneratedFigures/Runtime/).
   - [`Figures/plot_results.sh`](Figures/plot_results.sh) — rebuilds
     the runtime figures from **your** locally-produced CSVs in
     `Experiments/<exp>/results/` (after an `sbatch` run). The figures
     land **next to the CSVs that drove them**, i.e. inside each
     `Experiments/<exp>/results/` folder.
   - [`Figures/plot_all.sh`](Figures/plot_all.sh) — umbrella driver:
     illustrative groups (`AccessCost`, `Pebble_Game`, …) + `Peaks`
     (JSON refresh) + delegates to both of the above for runtime
     figures. See [Regenerate all figures](#regenerate-all-figures).

5. **[`PaperSnapshot/`](PaperSnapshot/)** — frozen, paper-canonical CSVs
   that produced the submitted figures. Mirrors `Experiments/<exp>/results/`
   layout so the same `plot_paper.py` reads from either root unchanged.

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

Three entry points — pick the one that matches what you need:

```bash
# Paper-canonical runtime figures only (reads PaperSnapshot/).
bash Figures/plot_paper_snapshot.sh

# YOUR runtime figures only (reads Experiments/<exp>/results/;
# run sbatch first).
bash Figures/plot_results.sh

# Umbrella — illustrative groups + Peaks + both of the above.
bash Figures/plot_all.sh
```

Output locations are kept deliberately separate so the two sets never
overwrite each other:

| Script | Reads from | Writes to |
|---|---|---|
| `plot_paper_snapshot.sh` | `PaperSnapshot/<exp>/results/*.csv` | `Figures/GeneratedFigures/Runtime/<stem>.{pdf,png}` |
| `plot_results.sh` | `Experiments/<exp>/results/*.csv` | **`Experiments/<exp>/results/<stem>.{pdf,png}`** (next to the CSVs) |

`plot_all.sh` also accepts group names for subsets:

```bash
bash Figures/plot_all.sh PaperSnapshot   # only plot_paper_snapshot.sh
bash Figures/plot_all.sh Results         # only plot_results.sh
bash Figures/plot_all.sh Runtime         # both of the above (no illustrative)
bash Figures/plot_all.sh Peaks           # refresh common/stream_peak.json
bash Figures/plot_all.sh AccessCost      # one illustrative group
bash Figures/plot_all.sh Peaks Results   # any combination
```

Missing CSVs (an experiment you haven't re-run, or an empty
`PaperSnapshot/<exp>/results/`) produce a one-line `[skip]` notice and
the script moves on — a single missing experiment never aborts the
sweep.

Illustrative-group outputs (`AccessCost`, `Pebble_Game`,
`LayoutTransformations`, `Replay`) land under
`Figures/GeneratedFigures/<group>/` unchanged. `Peaks` refreshes
`Experiments/common/stream_peak.json` from measured CSVs (no figures).

Prerequisites: `matplotlib`, `numpy`, `pandas`, `scipy`. On a cluster,
`source Experiments/common/activate.sh` first; elsewhere, a plain `pip
install matplotlib numpy pandas scipy` is enough.

### Reviewer workflow — producing your own runtime figures

Three steps to generate a set of reviewer-owned runtime figures and
compare them against the paper's:

```bash
# 1. One-time per machine (venv + DaCe + plotting deps).
bash Experiments/common/setup.sh

# 2. Run whichever experiments you want to reproduce. CSVs land in
#    Experiments/<exp>/results/{daint,beverin}/.
cd Experiments/E1_MatrixAdd && sbatch run_daint.sh   # or run_beverin.sh
cd ../E2_Conjugation      && sbatch run_daint.sh
# ... etc.

# 3. Plot what you just produced. PDFs/PNGs land next to your CSVs,
#    inside each Experiments/<exp>/results/ directory.
bash Figures/plot_results.sh
```

To compare against the submitted paper, regenerate the frozen set too:

```bash
bash Figures/plot_paper_snapshot.sh
# paper PDFs under Figures/GeneratedFigures/Runtime/
# your PDFs under Experiments/<exp>/results/
```

An experiment you didn't re-run in step 2 is simply skipped by
`plot_results.sh`; the paper side is unaffected. You can also invoke a
single experiment's plotter directly:

```bash
cd Experiments/E1_MatrixAdd && python plot_paper.py
# writes addition_paper_cpu_gpu.{pdf,png} into the cwd.
```

## Repo structure

```
SC26-Layout-AD/
├── README.md                 (this file)
├── Latex/                    (AD/AE appendix sources + built PDF)
├── Figures/                  (figure regeneration drivers + illustrative scripts)
│   ├── plot_paper_snapshot.sh  (paper-canonical runtime figures; reads PaperSnapshot/)
│   ├── plot_results.sh         (reviewer's runtime figures; reads Experiments/*/results/)
│   ├── plot_all.sh             (umbrella: illustrative + Peaks + both of the above)
│   ├── AccessCost/             (block / NUMA access-cost plots)
│   ├── Pebble_Game/            (pebble-game illustrations)
│   ├── LayoutTransformations/  (stage-by-stage layout transform figures)
│   ├── Replay/                 (stride replay figure)
│   └── GeneratedFigures/       (illustrative groups + paper-canonical runtime)
│       └── Runtime/            (Fig. 4, 8–11 from PaperSnapshot)
├── PaperSnapshot/            (frozen paper-canonical CSVs; mirrors Experiments/<exp>/results/ layout)
└── Experiments/              (post-sbatch CSVs + reviewer runtime figures land in <exp>/results/)
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
