# Figures — illustrative / supplemental figure generators

Standalone matplotlib scripts that produce the illustrative figures
used in the paper's proof section (Figures 2–3 in the main text) and
the supplemental material. These are **not** benchmarks; they run in
seconds on any machine with matplotlib + numpy available.

For the actual benchmark figures (Figures 4, 8–13) see
[`../Experiments/EX_*/plot_paper.py`](../Experiments/).

## Layout

```
Figures/
├── plot_all.sh                (regenerates every group)
├── README.md
├── AccessCost/                (block-access-cost + NUMA-cost figures)
│   ├── block_layouts.py
│   ├── plot_blocks_touched.py
│   ├── plot_access_cost_row_major.py
│   ├── plot_access_cost_tiled.py
│   └── plot_access_cost_numa.py
├── Pebble_Game/               (pebble-game illustrations)
│   ├── pebble.py
│   ├── pebble_game_1.py
│   └── pebble_game_2.py
├── LayoutTransformations/     (layout-transformation stage illustrations)
│   ├── layout_stages.py
│   ├── tiled_addition.py
│   └── pack_unpack.py
├── Replay/                    (stride replay figure)
│   └── replay.py
└── GeneratedFigures/          (ALL .pdf and .png outputs)
    ├── AccessCost/
    ├── Pebble_Game/
    ├── LayoutTransformations/
    └── Replay/
```

Generator scripts live under `Figures/<group>/`; every output is written
to `Figures/GeneratedFigures/<group>/`. The split keeps generator code
and byte-identical regenerated artefacts on separate paths so diffs and
git history stay clean.

## Regenerate

```bash
# All figures:
bash plot_all.sh

# Only one group:
bash plot_all.sh AccessCost
```

`plot_all.sh` `cd`s into the right `GeneratedFigures/<group>/` directory
before running each script, so the scripts' relative `plt.savefig(...)`
calls land in the right folder without any in-script edits.

## Requirements

- Python 3.10+
- numpy, matplotlib
- The `common/venv` from `../Experiments/common/setup.sh` is sufficient,
  but any environment with numpy + matplotlib works.

## Reviewer hint — `# TODO: VERSION`

No version-sensitive pins live here: the scripts use only numpy +
matplotlib and produce deterministic, style-only figures. If a
matplotlib / numpy upgrade moves a default (font size, tick rendering),
regenerate the whole folder in one pass so the figures stay
stylistically consistent.
