# PaperSnapshot — frozen CSVs used to produce the submitted figures

This directory is a frozen, paper-canonical snapshot of every per-experiment
`results/{platform}/*.csv` that went into Figures 4, 8–11 of the SC26
submission. It mirrors the `Experiments/<exp>/results/` layout exactly, so
`plot_paper.py` from each experiment works against it unchanged — only the
current working directory differs.

## Why it exists

- **Reproducibility audit.** A reviewer can compare what `plot_paper.py`
  produces from a fresh `sbatch run_*.sh` run against the bit-for-bit CSVs
  the submitted PDF was drawn from — catches accidental silent changes in
  the figure pipeline.
- **Plot-script development.** Edits to `plot_paper.py` can be validated
  immediately against the frozen paper numbers, without re-running any
  benchmarks.
- **Single-source of truth for the paper.** `Figures/plot_all.sh` by
  default regenerates the paper-canonical figures from these CSVs.

## Layout

```
PaperSnapshot/
├── E0_NUMA/results/{daint,beverin}/*.csv
├── E1_MatrixAdd/results/{daint,beverin}/*.csv
├── E2_Conjugation/results/{daint,beverin}/*.csv
├── E3_Transpose/results/{daint,beverin}/*.csv
├── E4_GAS/results/{daint,beverin}/*.csv
├── E5_USXX/results/{daint,beverin}/*.csv
└── E6_VelocityTendencies/loopnest_{1..6}/results/{daint,beverin}/*.csv
```

Each directory has the same filename / schema convention as
`Experiments/<exp>/results/{daint,beverin}/` — the point is that
`plot_paper.py` doesn't need to know which root it is reading.

## How `Figures/plot_all.sh` uses it

```bash
bash Figures/plot_all.sh Runtime
```

1. For each experiment, if `PaperSnapshot/<exp>/results/` is populated,
   `plot_paper.py` runs with `cwd = PaperSnapshot/<exp>/`. The resulting
   `.pdf` / `.png` files are gathered into
   `Figures/GeneratedFigures/Runtime/` — these are the **paper-canonical**
   figures.
2. The same `plot_paper.py` also runs with `cwd = Experiments/<exp>/`,
   producing the **new reproduction** from the local post-sbatch CSVs.
   Those figures are gathered into
   `Figures/GeneratedFigures/Runtime/new/`.

Reviewers comparing the two folders side-by-side see at a glance whether
their reproduction matches the paper.

If `PaperSnapshot/<exp>/results/` is empty (just the `.gitkeep`
placeholder), the paper step is skipped for that experiment and only the
new reproduction is plotted. An empty snapshot is the committed default
— populate it with the CSVs used for the final submission before
releasing the artifact.
