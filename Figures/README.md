# Figures — illustrative / supplemental figure generators

Standalone matplotlib scripts for the paper's proof figures (Figures
2–3) and supplemental illustrations. **Not benchmarks** — these run in
seconds anywhere matplotlib + numpy is installed.

For the runtime figures (Figures 4, 8–14) see
[`../Experiments/EX_*/plot_paper.py`](../Experiments/) and the two
sibling drivers `plot_paper_snapshot.sh` / `plot_results.sh`.

## Layout

```
Figures/
├── plot_all.sh                regenerates every group (or one: bash plot_all.sh AccessCost)
├── plot_paper_snapshot.sh     runtime figures from PaperSnapshot/
├── plot_results.sh            runtime figures from Experiments/<exp>/results/
├── matplotlibrc               pins DejaVu Sans repo-wide
├── AccessCost/                block-access-cost + NUMA-cost figures
├── Pebble_Game/               pebble-game illustrations
├── LayoutTransformations/     layout-transformation stage illustrations
├── Replay/                    stride replay figure
└── GeneratedFigures/<group>/  PDF / PNG outputs
```

Generator scripts live under `Figures/<group>/`; outputs go to
`Figures/GeneratedFigures/<group>/`. `plot_all.sh` `cd`s into the
right output folder before running each script so in-script
`plt.savefig(...)` paths land correctly.

## Requirements

Python 3.10+, numpy, matplotlib. The `common/venv` from
`../Experiments/common/setup.sh` is sufficient.

## Reviewer hint

No version-sensitive pins. If a matplotlib upgrade shifts a default,
regenerate the whole folder in one pass to keep figures stylistically
consistent.
