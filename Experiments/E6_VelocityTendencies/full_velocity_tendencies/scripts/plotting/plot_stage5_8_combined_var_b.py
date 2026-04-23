#!/usr/bin/env python3
"""
plot_stage8_gpu_broken.py

Option B: Broken Y-axis
Each platform column is split into two vertically stacked axes:
  top  = outlier region (narrow sliver)
  bot  = main data range (0 … cap)
with diagonal break marks between them.

1×2 layout: AMD MI300A (left) | NVIDIA GH200 (right)
Each panel: Config A (1/6 calls) | Config B (5/6 calls)
"""

import re, glob, os, argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from matplotlib.ticker import AutoMinorLocator, MaxNLocator, FormatStrFormatter
from scipy import stats as sp_stats
import matplotlib.gridspec as gridspec

# ── SC26 plot constants ────────────────────────────────────────────────────────
COL_BASELINE  = "#e67e22"
COL_OPTIMIZED = "#2980b9"

FONT_SUPTITLE = 15; FONT_SUBTITLE = 12; FONT_PANEL = 11
FONT_PCT = 10; FONT_LEGEND = 10; FONT_XTICK = 9; FONT_SEP = 9

SUBPLOT_W, SUBPLOT_H = 3.6, 2.8

VIOLIN_WIDTH = 0.9; VIOLIN_ALPHA = 0.75

CI_N_BOOT = 10_000; CI_FILL = "#FF69B4"
CI_OUTLINE_LW = 3.5; CI_FILL_LW = 1.8
CI_CAP_W = 0.08; CI_CAP_LW = 1.5; CI_CAP_OUTLINE_LW = 3
CI_MIN_HEIGHT_FRAC = 0.01

CAP_PERCENTILE = 97
CAP_MARGIN     = 1.15
BREAK_HEIGHT_RATIO = 0.18   # top axes = 18% of total height

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


def draw_break_marks(ax_top, ax_bot, d=0.015):
    """Draw diagonal break lines between two axes."""
    kwargs = dict(transform=ax_top.transAxes, color='k',
                  clip_on=False, lw=1)
    ax_top.plot((-d, +d), (-d, +d), **kwargs)
    ax_top.plot((1 - d, 1 + d), (-d, +d), **kwargs)

    kwargs.update(transform=ax_bot.transAxes)
    ax_bot.plot((-d, +d), (1 - d, 1 + d), **kwargs)
    ax_bot.plot((1 - d, 1 + d), (1 - d, 1 + d), **kwargs)


# ── Panel plotting (draws on BOTH top and bottom axes) ─────────────────────────
def plot_broken_panel(ax_bot, ax_top, folder, title, cap, outlier_lo, outlier_hi):
    """
    ax_bot: main range [0, cap]
    ax_top: outlier range [outlier_lo, outlier_hi]
    """
    group_centres = {7: 1.5, 9: 4.0}
    offsets = {BASELINE_KEY: -0.5, TARGET_KEY: 0.5}
    colors  = {BASELINE_KEY: COL_BASELINE, TARGET_KEY: COL_OPTIMIZED}

    plotted = []  # (pos, key, step, med, vmax, vmin)

    for step in STEPS:
        data = load_folder(folder, step)
        cx = group_centres[step]
        for key in [BASELINE_KEY, TARGET_KEY]:
            if key not in data:
                continue
            vals = np.array(data[key])
            pos = cx + offsets[key]
            med, ci_lo, ci_hi = bootstrap_ci_median(vals)

            # Plot violin on BOTH axes (matplotlib clips to ylim).
            # Canonical sampling: bw=scott, points=200; no outlier trim.
            for ax in (ax_bot, ax_top):
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

            # CI bar on bottom only (main range)
            min_ci = cap * CI_MIN_HEIGHT_FRAC
            ci_lo_d, ci_hi_d = ci_lo, ci_hi
            if ci_hi_d - ci_lo_d < min_ci:
                ci_lo_d = med - min_ci / 2
                ci_hi_d = med + min_ci / 2
            ax_bot.vlines(pos, ci_lo_d, ci_hi_d,
                          color="black", lw=CI_OUTLINE_LW, zorder=10)
            ax_bot.vlines(pos, ci_lo_d, ci_hi_d,
                          color=CI_FILL, lw=CI_FILL_LW, zorder=11)
            for cy in (ci_lo_d, ci_hi_d):
                ax_bot.hlines(cy, pos - CI_CAP_W, pos + CI_CAP_W,
                              color="black", lw=CI_CAP_OUTLINE_LW, zorder=10)
                ax_bot.hlines(cy, pos - CI_CAP_W, pos + CI_CAP_W,
                              color=CI_FILL, lw=CI_CAP_LW, zorder=11)

            plotted.append((pos, key, step, med,
                            float(np.max(vals)), float(np.min(vals))))

    # Median annotations on bottom axes
    for pos, key, step, med, vmax, vmin in plotted:
        if key == BASELINE_KEY:
            y_text = min(vmax * 1.12, cap * 0.94)
        else:
            y_text = min(vmax * 1.04, cap * 0.90)
        ax_bot.text(pos, y_text, f"med. {med:.2f} ms",
                    ha="center", va="bottom", fontsize=FONT_PCT,
                    fontstyle="italic", color=colors[key])

    # Speedup below blue violin
    for step in STEPS:
        bl  = [p for p in plotted if p[2] == step and p[1] == BASELINE_KEY]
        opt = [p for p in plotted if p[2] == step and p[1] == TARGET_KEY]
        if bl and opt:
            speedup = bl[0][3] / opt[0][3] if opt[0][3] > 0 else 0
            x = opt[0][0]
            y = opt[0][5]
            ax_bot.text(x, y * 0.96, f"{speedup:.2f}×",
                        ha="center", va="top", fontsize=FONT_PCT,
                        fontweight="bold", color=COL_OPTIMIZED)

    # Separator on both
    sep_x = (group_centres[7] + group_centres[9]) / 2
    for ax in (ax_bot, ax_top):
        ax.axvline(sep_x, color="gray", ls="--", lw=1.5, alpha=0.6)

    # Group labels (bottom only)
    for step in STEPS:
        cx = group_centres[step]
        ax_bot.text(cx, -0.08, STEP_LABEL[step], ha="center", va="top",
                    fontsize=FONT_SEP, transform=ax_bot.get_xaxis_transform())

    # X-ticks on bottom only
    tick_pos, tick_lab = [], []
    for step in STEPS:
        cx = group_centres[step]
        for key, label in [(BASELINE_KEY, "Orig."), (TARGET_KEY, "Opt.")]:
            tick_pos.append(cx + offsets[key])
            tick_lab.append(label)
    ax_bot.set_xticks(tick_pos)
    ax_bot.set_xticklabels(tick_lab, fontsize=FONT_XTICK)
    ax_top.set_xticks([])

    for ax in (ax_bot, ax_top):
        ax.set_xlim(0.3, 5.2)

    # Y-axis ranges
    ax_bot.set_ylim(bottom=0, top=cap)
    ax_top.set_ylim(bottom=outlier_lo, top=outlier_hi)

    # Remove inner spines
    ax_top.spines['bottom'].set_visible(False)
    ax_bot.spines['top'].set_visible(False)
    ax_top.tick_params(bottom=False)

    # Title on top axes
    ax_top.set_title(title, fontsize=FONT_PANEL, pad=4)

    # Y-label on bottom only
    ax_bot.set_ylabel("Time (ms)", fontsize=FONT_PANEL)

    # Grid on both
    for ax in (ax_bot, ax_top):
        ax.yaxis.set_major_formatter(FormatStrFormatter("%.1f"))
        ax.yaxis.set_minor_locator(AutoMinorLocator(2))
        ax.grid(True, axis="y", which="major", ls="--", alpha=0.25)
        ax.grid(True, axis="y", which="minor", ls=":", alpha=0.12)
        ax.set_axisbelow(True)

    ax_bot.yaxis.set_major_locator(MaxNLocator(nbins=5, min_n_ticks=5))
    ax_top.yaxis.set_major_locator(MaxNLocator(nbins=2))

    # Break marks
    draw_break_marks(ax_top, ax_bot)


# ── Main ───────────────────────────────────────────────────────────────────────
def none_or_str(v):
    return None if v.lower() == "none" else v

parser = argparse.ArgumentParser(
    description="1×2 violin (broken y-axis): stage-8 GPU.",
)
parser.add_argument("--gpu",        default="beverin_full_permutations_8", metavar="DIR", type=none_or_str)
parser.add_argument("--gpu2",       default="daint_full_permutations_8",   metavar="DIR", type=none_or_str)
parser.add_argument("--gpu-title",  default="AMD MI300A",                  metavar="STR")
parser.add_argument("--gpu2-title", default="NVIDIA GH200",                metavar="STR")
parser.add_argument("--out",        default="plots/violin_stage8_gpu_broken.png", metavar="FILE")
args = parser.parse_args()

for attr, path in [("gpu", args.gpu), ("gpu2", args.gpu2)]:
    if path and not os.path.isdir(path):
        parser.error(f"Folder '{path}' not found (--{attr.replace('_','-')}).")

plt.rcParams.update({
    "font.size": 11, "axes.titlesize": FONT_PANEL, "axes.labelsize": FONT_PANEL,
    "xtick.labelsize": FONT_XTICK, "ytick.labelsize": FONT_XTICK,
    "legend.fontsize": FONT_LEGEND,
})

# Compute global cap and outlier range from all data
all_vals_global = []
PLATFORMS = [
    (args.gpu,  args.gpu_title,  0),
    (args.gpu2, args.gpu2_title, 1),
]
for folder, _, _ in PLATFORMS:
    if folder and os.path.isdir(folder):
        for step in STEPS:
            d = load_folder(folder, step)
            for k in KEEP_KEYS:
                if k in d:
                    all_vals_global.extend(d[k])

all_vals_global = np.array(all_vals_global)
cap = float(np.percentile(all_vals_global, CAP_PERCENTILE) * CAP_MARGIN)
global_max = float(np.max(all_vals_global))

# Outlier region: cap to global_max with padding
outlier_lo = cap * 0.95
outlier_hi = global_max * 1.08

# ── Figure with GridSpec ──────────────────────────────────────────────────────
fig = plt.figure(figsize=(SUBPLOT_W * 2, SUBPLOT_H + 1.4))
gs = gridspec.GridSpec(2, 2,
                       height_ratios=[BREAK_HEIGHT_RATIO, 1 - BREAK_HEIGHT_RATIO],
                       hspace=0.08,
                       wspace=0.25)

fig.suptitle("Velocity Tendencies — Layout Permutation (GPU)",
             fontsize=FONT_SUPTITLE, y=0.97)

for folder, plat_title, col in PLATFORMS:
    ax_top = fig.add_subplot(gs[0, col])
    ax_bot = fig.add_subplot(gs[1, col])
    if folder is None or not os.path.isdir(folder):
        ax_bot.set_title(f"{plat_title}\n[no data]", fontsize=FONT_PANEL)
        continue
    plot_broken_panel(ax_bot, ax_top, folder, plat_title,
                      cap, outlier_lo, outlier_hi)
    if col > 0:
        ax_bot.set_ylabel("")

# Legend
handles = [
    Patch(facecolor=COL_BASELINE,  edgecolor="black", label="Original Layout"),
    Patch(facecolor=COL_OPTIMIZED, edgecolor="black", label="Optimized Layout"),
]
fig.legend(handles=handles, loc="lower center",
           bbox_to_anchor=(0.5, 0.0), ncol=2, framealpha=0.9,
           fontsize=FONT_LEGEND)

fig.subplots_adjust(bottom=0.12, top=0.90)
os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
fig.savefig(args.out, dpi=200, bbox_inches="tight")
fig.savefig(args.out.replace(".png", ".pdf"), bbox_inches="tight")
plt.close(fig)
print(f"Saved {args.out}")