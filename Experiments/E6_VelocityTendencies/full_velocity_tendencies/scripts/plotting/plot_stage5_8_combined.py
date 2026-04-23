#!/usr/bin/env python3
"""
plot_stage8_gpu.py

1×2 violin plot for stage-8 GPU results.
  col 0 : AMD MI300A (Beverin)
  col 1 : NVIDIA GH200 (Daint)

Each panel has two groups separated by a dashed line:
  Config A (5/6 calls) = step 7   |   Config B (1/6 calls) = step 9
with "Original Layout" (orange) and "Optimized Layout" (blue) violins.
"""

import re, glob, os, argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from matplotlib.ticker import AutoMinorLocator, MaxNLocator, FormatStrFormatter
from scipy import stats as sp_stats

# ── SC26 plot constants ────────────────────────────────────────────────────────
COL_BASELINE  = "#e67e22"   # orange  – Original Layout
COL_OPTIMIZED = "#2980b9"   # blue    – Optimized Layout

FONT_SUPTITLE = 15; FONT_SUBTITLE = 12; FONT_PANEL = 11
FONT_PCT = 10; FONT_LEGEND = 10; FONT_XTICK = 9; FONT_SEP = 9

SUBPLOT_W, SUBPLOT_H = 3.6, 2.8

IQR_K = 3.0; MIN_SAMPLES = 4
VIOLIN_WIDTH = 0.9; VIOLIN_ALPHA = 0.75

CI_N_BOOT = 10_000; CI_FILL = "#FF69B4"
CI_OUTLINE_LW = 3.5; CI_FILL_LW = 1.8
CI_CAP_W = 0.08; CI_CAP_LW = 1.5; CI_CAP_OUTLINE_LW = 3
CI_MIN_HEIGHT_FRAC = 0.01

PATTERN = re.compile(r"\[Timer\] Elapsed time: ([\d.]+) ms")
BASELINE_KEY = "unpermuted"
TARGET_KEY   = "nlev_first_shuffled"
KEEP_KEYS    = {BASELINE_KEY, TARGET_KEY}

STEPS = [7, 9]
STEP_LABEL = {
    7: "Config A (1/6 calls)",
    9: "Config B (5/6 calls)",
}

# ── Helpers ────────────────────────────────────────────────────────────────────
def bootstrap_ci_median(vals, alpha=0.05):
    arr = np.asarray(vals)
    if len(arr) < 3:
        med = np.median(arr)
        return med, med, med
    med = np.median(arr)
    try:
        res = sp_stats.bootstrap(
            (arr,), np.median, n_resamples=CI_N_BOOT,
            confidence_level=1 - alpha, method="BCa",
        )
        return med, res.confidence_interval.low, res.confidence_interval.high
    except Exception:
        try:
            res = sp_stats.bootstrap(
                (arr,), np.median, n_resamples=CI_N_BOOT,
                confidence_level=1 - alpha, method="percentile",
            )
            return med, res.confidence_interval.low, res.confidence_interval.high
        except Exception:
            return med, med, med


# ── Data loading ───────────────────────────────────────────────────────────────
def load_folder(folder: str, step: int) -> dict[str, list[float]]:
    raw: dict[str, list[float]] = {}
    for f in glob.glob(os.path.join(folder, "*.txt")):
        basename = os.path.basename(f).replace(".txt", "")
        has_step = re.search(r"_step\d+$", basename)
        if has_step and not basename.endswith(f"_step{step}"):
            continue
        name = re.sub(r"_step\d+$", "", basename)
        if name not in KEEP_KEYS:
            continue
        with open(f) as fh:
            times = [float(m.group(1)) for line in fh if (m := PATTERN.search(line))]
        if times:
            raw.setdefault(name, []).extend(times)
    return raw


# ── Panel plotting ─────────────────────────────────────────────────────────────
def plot_panel(ax, folder: str, title: str) -> None:
    """Plot both Config A and Config B as grouped violins in one panel."""
    group_centres = {7: 1.5, 9: 4.0}
    offsets = {BASELINE_KEY: -0.5, TARGET_KEY: 0.5}
    colors  = {BASELINE_KEY: COL_BASELINE, TARGET_KEY: COL_OPTIMIZED}

    plotted = []  # (pos, key, step, med)

    for step in STEPS:
        data = load_folder(folder, step)
        cx = group_centres[step]
        for key in [BASELINE_KEY, TARGET_KEY]:
            if key not in data:
                continue
            vals = np.array(data[key])
            pos = cx + offsets[key]
            med, ci_lo, ci_hi = bootstrap_ci_median(vals)

            # Violin (canonical sampling: bw=scott, points=200, shared
            # with every other plot in the artifact).
            vp = ax.violinplot(
                [vals], positions=[pos], widths=VIOLIN_WIDTH,
                showmeans=True, showmedians=True, showextrema=False,
                bw_method="scott", points=200,
            )
            for body in vp["bodies"]:
                body.set_facecolor(colors[key])
                body.set_edgecolor("black")
                body.set_alpha(VIOLIN_ALPHA)
                body.set_zorder(2)
            vp["cmeans"].set_color("black");   vp["cmeans"].set_zorder(3)
            vp["cmedians"].set_color("white"); vp["cmedians"].set_zorder(3)

            # CI bar (black outline underneath, pink on top)
            plot_top = max(vals) * 1.3
            min_ci = plot_top * CI_MIN_HEIGHT_FRAC
            if ci_hi - ci_lo < min_ci:
                ci_lo = med - min_ci / 2
                ci_hi = med + min_ci / 2
            ax.vlines(pos, ci_lo, ci_hi,
                      color="black", lw=CI_OUTLINE_LW, zorder=10)
            ax.vlines(pos, ci_lo, ci_hi,
                      color=CI_FILL, lw=CI_FILL_LW, zorder=11)
            for cy in (ci_lo, ci_hi):
                ax.hlines(cy, pos - CI_CAP_W, pos + CI_CAP_W,
                          color="black", lw=CI_CAP_OUTLINE_LW, zorder=10)
                ax.hlines(cy, pos - CI_CAP_W, pos + CI_CAP_W,
                          color=CI_FILL, lw=CI_CAP_LW, zorder=11)

            plotted.append((pos, key, step, med, float(np.max(vals)), float(np.min(vals))))

    # Median annotations (italics) above each violin
    # Orange (baseline) higher than blue (optimized)
    for pos, key, step, med, vmax, vmin in plotted:
        if key == BASELINE_KEY:
            y_text = vmax * 1.12
        else:
            y_text = vmax * 1.04
        ax.text(pos, y_text, f"med. {med:.2f} ms",
                ha="center", va="bottom", fontsize=FONT_PCT,
                fontstyle="italic", color=colors[key])

    # Speedup annotations directly below the blue (optimized) violin min
    for step in STEPS:
        bl  = [p for p in plotted if p[2] == step and p[1] == BASELINE_KEY]
        opt = [p for p in plotted if p[2] == step and p[1] == TARGET_KEY]
        if bl and opt:
            speedup = bl[0][3] / opt[0][3] if opt[0][3] > 0 else 0
            x = opt[0][0]            # below the blue violin
            y = opt[0][5]            # vmin of blue
            ax.text(x, y * 0.96, f"{speedup:.2f}×",
                    ha="center", va="top", fontsize=FONT_PCT,
                    fontweight="bold", color=COL_OPTIMIZED)

    # Separator between groups
    sep_x = (group_centres[7] + group_centres[9]) / 2
    ax.axvline(sep_x, color="gray", ls="--", lw=1.5, alpha=0.6)

    # Group labels at bottom
    for step in STEPS:
        cx = group_centres[step]
        ax.text(cx, -0.08, STEP_LABEL[step], ha="center", va="top",
                fontsize=FONT_SEP, transform=ax.get_xaxis_transform())

    # X-ticks
    tick_pos, tick_lab = [], []
    for step in STEPS:
        cx = group_centres[step]
        for key, label in [(BASELINE_KEY, "Orig."), (TARGET_KEY, "Opt.")]:
            tick_pos.append(cx + offsets[key])
            tick_lab.append(label)
    ax.set_xticks(tick_pos)
    ax.set_xticklabels(tick_lab, fontsize=FONT_XTICK)
    ax.set_xlim(0.3, 5.2)

    # Y-axis: start at 0
    ax.set_ylim(bottom=0)
    ax.set_ylabel("Time (ms)", fontsize=FONT_PANEL)
    ax.set_title(title, fontsize=FONT_PANEL, pad=4)

    # Grid
    ax.yaxis.set_major_locator(MaxNLocator(nbins=5, min_n_ticks=5))
    ax.yaxis.set_major_formatter(FormatStrFormatter("%.1f"))
    ax.yaxis.set_minor_locator(AutoMinorLocator(2))
    ax.grid(True, axis="y", which="major", ls="--", alpha=0.25)
    ax.grid(True, axis="y", which="minor", ls=":", alpha=0.12)
    ax.set_axisbelow(True)
    ax.set_box_aspect(SUBPLOT_H / SUBPLOT_W)


# ── Main ───────────────────────────────────────────────────────────────────────
def none_or_str(v):
    return None if v.lower() == "none" else v

parser = argparse.ArgumentParser(
    description="1×2 violin: stage-8 GPU (MI300A left, GH200 right).",
)
parser.add_argument("--gpu",        default="beverin_full_permutations_8", metavar="DIR", type=none_or_str)
parser.add_argument("--gpu2",       default="daint_full_permutations_8",   metavar="DIR", type=none_or_str)
parser.add_argument("--gpu-title",  default="AMD MI300A",                  metavar="STR")
parser.add_argument("--gpu2-title", default="NVIDIA GH200",                metavar="STR")
parser.add_argument("--out",        default="plots/violin_stage8_gpu.png", metavar="FILE")
args = parser.parse_args()

for attr, path in [("gpu", args.gpu), ("gpu2", args.gpu2)]:
    if path and not os.path.isdir(path):
        parser.error(f"Folder '{path}' not found (--{attr.replace('_','-')}).")

plt.rcParams.update({
    "font.size": 11, "axes.titlesize": FONT_PANEL, "axes.labelsize": FONT_PANEL,
    "xtick.labelsize": FONT_XTICK, "ytick.labelsize": FONT_XTICK,
    "legend.fontsize": FONT_LEGEND,
})

nrows, ncols = 1, 2
fig_h = SUBPLOT_H * nrows + 1.2   # +legend +suptitle padding
fig, axes = plt.subplots(nrows, ncols,
                         figsize=(SUBPLOT_W * ncols, fig_h),
                         sharey=True)

fig.suptitle("Velocity Tendencies — Layout Permutation (GPU)",
             fontsize=FONT_SUPTITLE, y=0.95)

PLATFORMS = [
    (args.gpu,  args.gpu_title,  0),   # AMD left
    (args.gpu2, args.gpu2_title, 1),   # NVIDIA right
]

for folder, plat_title, col in PLATFORMS:
    ax = axes[col]
    if folder is None or not os.path.isdir(folder):
        ax.set_title(f"{plat_title}\n[no data]", fontsize=FONT_PANEL)
        continue
    plot_panel(ax, folder, plat_title)

axes[1].set_ylabel("")

# Legend
handles = [
    Patch(facecolor=COL_BASELINE,  edgecolor="black", label="Original Layout"),
    Patch(facecolor=COL_OPTIMIZED, edgecolor="black", label="Optimized Layout"),
]
fig.legend(handles=handles, loc="lower center",
           bbox_to_anchor=(0.5, 0.0), ncol=2, framealpha=0.9,
           fontsize=FONT_LEGEND)

fig.tight_layout(rect=[0, 0.05, 1, 0.90])
os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
fig.savefig(args.out, dpi=200, bbox_inches="tight")
fig.savefig(args.out.replace(".png", ".pdf"), bbox_inches="tight")
plt.close(fig)
print(f"Saved {args.out}")