#!/usr/bin/env python3
"""
plot_stage8_gpu_cap.py

Option A: Cap + Annotation
Clips y-axis at a sensible cap (p97 × 1.15 of the "good" data).
Annotates how many outlier points exceed the cap per violin.

1×2 layout: AMD MI300A (left) | NVIDIA GH200 (right)
Each panel: Config A (1/6 calls) | Config B (5/6 calls)
"""

import re, glob, os, argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from matplotlib.ticker import AutoMinorLocator, MaxNLocator, FormatStrFormatter
from scipy import stats as sp_stats

# ── SC26 plot constants ────────────────────────────────────────────────────────
COL_BASELINE  = "#e67e22"
COL_OPTIMIZED = "#2980b9"

FONT_SUPTITLE = 15; FONT_SUBTITLE = 12; FONT_PANEL = 11
FONT_PCT = 10; FONT_LEGEND = 10; FONT_XTICK = 9; FONT_SEP = 9

SUBPLOT_W, SUBPLOT_H = 3.6, 4.4

VIOLIN_WIDTH = 0.9; VIOLIN_ALPHA = 0.75

CI_N_BOOT = 10_000; CI_FILL = "#FF69B4"
CI_OUTLINE_LW = 3.5; CI_FILL_LW = 1.8
CI_CAP_W = 0.08; CI_CAP_LW = 1.5; CI_CAP_OUTLINE_LW = 3
CI_MIN_HEIGHT_FRAC = 0.01

# Cap heuristic
CAP_PERCENTILE = 99      # compute cap from this percentile
CAP_MARGIN     = 1.85    # multiply percentile by this

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


# ── Panel plotting ─────────────────────────────────────────────────────────────
def plot_panel(ax, folder: str, title: str, forced_cap=None, is_amd=False) -> float:
    """Plot grouped violins. Returns the computed cap for sharey alignment."""
    group_centres = {7: 1.5, 9: 4.0}
    offsets = {BASELINE_KEY: -0.5, TARGET_KEY: 0.5}
    colors  = {BASELINE_KEY: COL_BASELINE, TARGET_KEY: COL_OPTIMIZED}

    # First pass: collect all values to compute cap
    all_raw = {}
    for step in STEPS:
        all_raw[step] = load_folder(folder, step)

    every_val = []
    for step in STEPS:
        for key in [BASELINE_KEY, TARGET_KEY]:
            if key in all_raw[step]:
                every_val.extend(all_raw[step][key])
    every_val = np.array(every_val)

    if forced_cap is not None:
        cap = forced_cap
    else:
        cap = np.percentile(every_val, CAP_PERCENTILE) * CAP_MARGIN

    # Second pass: plot
    plotted = []  # (pos, key, step, med, vmax_clipped, vmin)

    for step in STEPS:
        data = all_raw[step]
        cx = group_centres[step]
        for key in [BASELINE_KEY, TARGET_KEY]:
            if key not in data:
                continue
            vals = np.array(data[key])
            pos = cx + offsets[key]
            med, ci_lo, ci_hi = bootstrap_ci_median(vals)

            # Count outliers above cap
            n_above = int(np.sum(vals > cap))
            vmax_raw = float(np.max(vals))

            # Violin (canonical sampling: bw=scott, points=200). All
            # data is shown; ylim clips visually but no outliers are
            # removed from the distribution estimator.
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

            # CI bar
            min_ci = cap * CI_MIN_HEIGHT_FRAC
            ci_lo_d, ci_hi_d = ci_lo, ci_hi
            if ci_hi_d - ci_lo_d < min_ci:
                ci_lo_d = med - min_ci / 2
                ci_hi_d = med + min_ci / 2
            ax.vlines(pos, ci_lo_d, ci_hi_d,
                      color="black", lw=CI_OUTLINE_LW, zorder=10)
            ax.vlines(pos, ci_lo_d, ci_hi_d,
                      color=CI_FILL, lw=CI_FILL_LW, zorder=11)
            for cy in (ci_lo_d, ci_hi_d):
                ax.hlines(cy, pos - CI_CAP_W, pos + CI_CAP_W,
                          color="black", lw=CI_CAP_OUTLINE_LW, zorder=10)
                ax.hlines(cy, pos - CI_CAP_W, pos + CI_CAP_W,
                          color=CI_FILL, lw=CI_CAP_LW, zorder=11)

            vmax_vis = min(vmax_raw, cap)
            p99 = float(np.percentile(vals, 99))
            plotted.append((pos, key, step, med, vmax_vis, float(np.min(vals)),
                            n_above, vmax_raw, p99))

    # Outlier annotations at top of clipped violins
    for pos, key, step, med, vmax_vis, vmin, n_above, vmax_raw, p99 in plotted:
        if n_above > 0:
            ax.text(pos + 0.35, cap * 0.98,
                f"▲ {n_above} pt{'s' if n_above > 1 else ''}\n> {vmax_raw:.1f}",
                ha="center", va="top", fontsize=9,
                fontstyle="italic", color="#888888")

    # Median annotations (italics) above each violin
    # Default: above vmax.  MI300A baseline: above p99.
    for pos, key, step, med, vmax_vis, vmin, n_above, vmax_raw, p99 in plotted:
        if key == BASELINE_KEY and is_amd:
            y_text = p99 * 1.32
            pos += 0.5
        else:
            y_text = vmax_vis * 1.04
        y_text = min(y_text, cap * 0.94)
        ax.text(pos, y_text, f"med.\n{med:.2f} ms",
                ha="center", va="bottom", fontsize=FONT_PCT,
                fontstyle="italic", color=colors[key])

    # Dashed line from orange median to blue median per group
    for step in STEPS:
        bl  = [p for p in plotted if p[2] == step and p[1] == BASELINE_KEY]
        opt = [p for p in plotted if p[2] == step and p[1] == TARGET_KEY]
        if bl and opt:
            x_bl, med_bl = bl[0][0], bl[0][3]
            x_opt, med_opt = opt[0][0], opt[0][3]
            ax.plot([x_bl, x_opt+0.4], [med_bl, med_bl],
                    color=COL_BASELINE, ls="--", lw=1.5, alpha=0.5, zorder=4)

    # Speedup below blue violin
    for step in STEPS:
        bl  = [p for p in plotted if p[2] == step and p[1] == BASELINE_KEY]
        opt = [p for p in plotted if p[2] == step and p[1] == TARGET_KEY]
        if bl and opt:
            speedup = bl[0][3] / opt[0][3] if opt[0][3] > 0 else 0
            x = opt[0][0]
            y = opt[0][5]   # vmin
            ax.text(x, y * 0.96, f"{speedup:.2f}×",
                    ha="center", va="top", fontsize=FONT_PCT,
                    fontweight="bold", color=COL_OPTIMIZED)

    # Separator
    sep_x = (group_centres[7] + group_centres[9]) / 2
    ax.axvline(sep_x, color="gray", ls="--", lw=1.5, alpha=0.6)

    # Group labels
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

    ax.set_ylim(bottom=0.4, top=cap)
    ax.set_ylabel("Time (ms)", fontsize=FONT_PANEL)
    ax.set_title(title, fontsize=FONT_PANEL, pad=4)

    # Grid
    ax.yaxis.set_major_locator(MaxNLocator(nbins=7, min_n_ticks=7))
    ax.yaxis.set_major_formatter(FormatStrFormatter("%.1f"))
    ax.yaxis.set_minor_locator(AutoMinorLocator(2))
    ax.grid(True, axis="y", which="major", ls="--", alpha=0.25)
    ax.grid(True, axis="y", which="minor", ls=":", alpha=0.12)
    ax.set_axisbelow(True)
    ax.set_box_aspect(SUBPLOT_H / SUBPLOT_W)

    return cap


# ── Main ───────────────────────────────────────────────────────────────────────
def none_or_str(v):
    return None if v.lower() == "none" else v

parser = argparse.ArgumentParser(
    description="1×2 violin (cap+annotation): stage-8 GPU.",
)
parser.add_argument("--gpu",        default="beverin_full_permutations_8", metavar="DIR", type=none_or_str)
parser.add_argument("--gpu2",       default="daint_full_permutations_8",   metavar="DIR", type=none_or_str)
parser.add_argument("--gpu-title",  default="MI300A GPU",                  metavar="STR")
parser.add_argument("--gpu2-title", default="GH200 Hopper GPU",                metavar="STR")
parser.add_argument("--out",        default="plots/violin_stage8_gpu_cap.png", metavar="FILE")
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
fig_h = SUBPLOT_H * nrows + 1.5
fig, axes = plt.subplots(nrows, ncols,
                         figsize=(SUBPLOT_W * ncols, fig_h))

fig.suptitle("Velocity Tendencies — Data Layout Impact on GPUs",
             fontsize=FONT_SUPTITLE, y=0.89)
fig.text(0.5, 0.845,
         "y-axis capped at p99; outlier counts noted where applicable",
         ha='center', va='top', fontsize=FONT_SUBTITLE, color='dimgray')

PLATFORMS = [
    (args.gpu,  args.gpu_title,  0),
    (args.gpu2, args.gpu2_title, 1),
]

# Each panel computes its own cap (no shared y-axis)
for folder, plat_title, col in PLATFORMS:
    ax = axes[col]
    if folder is None or not os.path.isdir(folder):
        ax.set_title(f"{plat_title}\n[no data]", fontsize=FONT_PANEL)
        continue
    plot_panel(ax, folder, plat_title, forced_cap=None, is_amd=(col == 0))

axes[1].set_ylabel("")

handles = [
    Patch(facecolor=COL_BASELINE,  edgecolor="black", label="Original Layout"),
    Patch(facecolor=COL_OPTIMIZED, edgecolor="black", label="Optimized Layout"),
]
fig.legend(handles=handles, loc="lower center",
           bbox_to_anchor=(0.5, -0.055), ncol=2, framealpha=0.9,
           fontsize=FONT_LEGEND)

fig.tight_layout(rect=[0, 0.08, 1, 0.82])
os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
fig.savefig(args.out, dpi=200, bbox_inches="tight")
fig.savefig(args.out.replace(".png", ".pdf"), bbox_inches="tight")
plt.close(fig)
print(f"Saved {args.out}")