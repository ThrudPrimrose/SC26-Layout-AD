#!/usr/bin/env python3
"""
plot_paper.py -- E7 full velocity-tendencies, paper-canonical Fig 14.

Identical visual + statistical treatment as
``Experiments/E8_LegacyVT/plot_paper.py`` (which is the verbatim
icon-artifacts ``plot_stage5_8_combined_var_a.py`` source-of-truth):
1×2 grid (MI300A | GH200), per-panel "Config A (1/6 calls) | Config B
(5/6 calls)" groups, Original (orange) vs Optimized (blue) violins,
median annotations, BCa bootstrap CIs, p99 cap, dashed median bridge,
speedup factor under the optimized violin.

E7 output layout differs from E8's:

    E8 native:  ``<plat>_full_permutations_8/<cfg>_<shuf|unshuf>[_step<N>].txt``
    E7 native:  ``results/<plat>/<cfg>/ts<N>/run.txt``

This script translates E7's nested layout into the same in-memory
shape so the rest of the plotting code is verbatim. The E8 BASELINE_KEY
(``unpermuted``) and TARGET_KEY (``nlev_first_shuffled``) map to E7's
``unpermuted_lv0_sm0`` (no permutation, no map shuffle) and
``nlev_first_lv0_sm1`` (level-first permutation, with map shuffle).
Override via ``--baseline`` / ``--target``.

Usage::

    python plot_paper.py
    python plot_paper.py --baseline unpermuted_lv0_sm0 --target nlev_first_lv1_sm1
    python plot_paper.py --gpu results/beverin --gpu2 results/daint
"""

import re, glob, os, argparse
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from matplotlib.ticker import AutoMinorLocator, MaxNLocator, FormatStrFormatter
from scipy import stats as sp_stats

# ── SC26 plot constants (kept in lock-step with E8/plot_paper.py) ────────────
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

CAP_PERCENTILE = 99
CAP_MARGIN     = 1.85

# Two timer-line shapes the binary may emit. We accept both, in ms.
PATTERN_MS = re.compile(r"\[Timer\] Elapsed time: ([\d.]+) ms")
PATTERN_BODY_MS = re.compile(r"velocity body:\s+([0-9.eE+-]+)\s+ms")
PATTERN_US = re.compile(r"Timer\s+velocity_no_nproma_if_prop_lvn_only_\d+_istep_\d+\s+took\s+(\d+)\s+us")

STEPS = [7, 9]
STEP_LABEL = {
    7: "Config A (1/6 calls)",
    9: "Config B (5/6 calls)",
}


# ── Helpers ──────────────────────────────────────────────────────────────────
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


def parse_one_log(path):
    """Return all per-rep elapsed-time samples in ms."""
    out = []
    try:
        with open(path) as fh:
            for line in fh:
                m = PATTERN_MS.search(line)
                if m:
                    out.append(float(m.group(1))); continue
                m = PATTERN_BODY_MS.search(line)
                if m:
                    out.append(float(m.group(1))); continue
                m = PATTERN_US.search(line)
                if m:
                    out.append(int(m.group(1)) * 1e-3); continue
    except OSError:
        pass
    return out


def load_folder(folder, step, baseline_key, target_key):
    """Return ``{key: [ms samples]}`` for a single (folder, step).

    ``folder`` may be either:
      - flat directory of ``<cfg>_<shuf|unshuf>[_step<N>].txt`` (E8/icon-artifacts), OR
      - nested ``<cfg>/ts<N>/run.txt`` tree (E7).

    Both are inspected; we keep only the two configs the panel cares
    about (``baseline_key`` and ``target_key``).
    """
    keep = {baseline_key, target_key}
    raw = {}
    p = Path(folder)
    if not p.is_dir():
        return raw

    # --- Flat layout (E8 native + icon-artifacts) ---
    for f in glob.glob(os.path.join(folder, "*.txt")):
        basename = os.path.basename(f).replace(".txt", "")
        has_step = re.search(r"_step\d+$", basename)
        if has_step and not basename.endswith(f"_step{step}"):
            continue
        name = re.sub(r"_step\d+$", "", basename)
        if name not in keep:
            continue
        times = parse_one_log(f)
        if times:
            raw.setdefault(name, []).extend(times)

    # --- Nested layout (E7 native) ---
    # ``folder`` here points at e.g. ``results/daint/``; iterate
    # ``<cfg>/ts<step>/run.txt`` for the requested step.
    for cfg_dir in sorted(p.iterdir()):
        if not cfg_dir.is_dir():
            continue
        cfg = cfg_dir.name
        if cfg not in keep:
            continue
        run_txt = cfg_dir / f"ts{step}" / "run.txt"
        if not run_txt.is_file():
            continue
        times = parse_one_log(run_txt)
        if times:
            raw.setdefault(cfg, []).extend(times)

    return raw


# ── Panel plotting (verbatim shape; differs only in the load_folder hook) ────
def plot_panel(ax, folder, title, baseline_key, target_key, forced_cap=None, is_amd=False):
    group_centres = {7: 1.5, 9: 4.0}
    offsets = {baseline_key: -0.5, target_key: 0.5}
    colors  = {baseline_key: COL_BASELINE, target_key: COL_OPTIMIZED}

    all_raw = {}
    for step in STEPS:
        all_raw[step] = load_folder(folder, step, baseline_key, target_key)

    every_val = []
    for step in STEPS:
        for key in [baseline_key, target_key]:
            if key in all_raw[step]:
                every_val.extend(all_raw[step][key])
    every_val = np.array(every_val) if every_val else np.array([1.0])

    cap = forced_cap if forced_cap is not None \
        else float(np.percentile(every_val, CAP_PERCENTILE) * CAP_MARGIN)

    plotted = []
    for step in STEPS:
        data = all_raw[step]
        cx = group_centres[step]
        for key in [baseline_key, target_key]:
            if key not in data:
                continue
            vals = np.array(data[key])
            pos = cx + offsets[key]
            med, ci_lo, ci_hi = bootstrap_ci_median(vals)
            n_above = int(np.sum(vals > cap))
            vmax_raw = float(np.max(vals))

            vp = ax.violinplot(
                [vals], positions=[pos], widths=VIOLIN_WIDTH,
                showmeans=True, showmedians=True, showextrema=False,
            )
            for body in vp["bodies"]:
                body.set_facecolor(colors[key])
                body.set_edgecolor("black")
                body.set_alpha(VIOLIN_ALPHA)
                body.set_zorder(2)
            vp["cmeans"].set_color("black");   vp["cmeans"].set_zorder(3)
            vp["cmedians"].set_color("white"); vp["cmedians"].set_zorder(3)

            min_ci = cap * CI_MIN_HEIGHT_FRAC
            ci_lo_d, ci_hi_d = ci_lo, ci_hi
            if ci_hi_d - ci_lo_d < min_ci:
                ci_lo_d = med - min_ci / 2
                ci_hi_d = med + min_ci / 2
            ax.vlines(pos, ci_lo_d, ci_hi_d, color="black", lw=CI_OUTLINE_LW, zorder=10)
            ax.vlines(pos, ci_lo_d, ci_hi_d, color=CI_FILL,  lw=CI_FILL_LW,    zorder=11)
            for cy in (ci_lo_d, ci_hi_d):
                ax.hlines(cy, pos - CI_CAP_W, pos + CI_CAP_W,
                          color="black", lw=CI_CAP_OUTLINE_LW, zorder=10)
                ax.hlines(cy, pos - CI_CAP_W, pos + CI_CAP_W,
                          color=CI_FILL,  lw=CI_CAP_LW,        zorder=11)

            vmax_vis = min(vmax_raw, cap)
            p99 = float(np.percentile(vals, 99))
            plotted.append((pos, key, step, med, vmax_vis, float(np.min(vals)),
                            n_above, vmax_raw, p99))

    for pos, key, step, med, vmax_vis, vmin, n_above, vmax_raw, p99 in plotted:
        if n_above > 0:
            ax.text(pos + 0.35, cap * 0.98,
                f"▲ {n_above} pt{'s' if n_above > 1 else ''}\n> {vmax_raw:.1f}",
                ha="center", va="top", fontsize=9,
                fontstyle="italic", color="#888888")

    for pos, key, step, med, vmax_vis, vmin, n_above, vmax_raw, p99 in plotted:
        if key == baseline_key and is_amd:
            y_text = p99 * 1.32
            pos += 0.5
        else:
            y_text = vmax_vis * 1.04
        y_text = min(y_text, cap * 0.94)
        ax.text(pos, y_text, f"med.\n{med:.2f} ms",
                ha="center", va="bottom", fontsize=FONT_PCT,
                fontstyle="italic", color=colors[key])

    for step in STEPS:
        bl  = [p for p in plotted if p[2] == step and p[1] == baseline_key]
        opt = [p for p in plotted if p[2] == step and p[1] == target_key]
        if bl and opt:
            x_bl, med_bl = bl[0][0], bl[0][3]
            x_opt, med_opt = opt[0][0], opt[0][3]
            ax.plot([x_bl, x_opt+0.4], [med_bl, med_bl],
                    color=COL_BASELINE, ls="--", lw=1.5, alpha=0.5, zorder=4)

    for step in STEPS:
        bl  = [p for p in plotted if p[2] == step and p[1] == baseline_key]
        opt = [p for p in plotted if p[2] == step and p[1] == target_key]
        if bl and opt:
            speedup = bl[0][3] / opt[0][3] if opt[0][3] > 0 else 0
            x = opt[0][0]
            y = opt[0][5]
            ax.text(x, y * 0.96, f"{speedup:.2f}×",
                    ha="center", va="top", fontsize=FONT_PCT,
                    fontweight="bold", color=COL_OPTIMIZED)

    sep_x = (group_centres[7] + group_centres[9]) / 2
    ax.axvline(sep_x, color="gray", ls="--", lw=1.5, alpha=0.6)

    for step in STEPS:
        cx = group_centres[step]
        ax.text(cx, -0.08, STEP_LABEL[step], ha="center", va="top",
                fontsize=FONT_SEP, transform=ax.get_xaxis_transform())

    tick_pos, tick_lab = [], []
    for step in STEPS:
        cx = group_centres[step]
        for key, label in [(baseline_key, "Orig."), (target_key, "Opt.")]:
            tick_pos.append(cx + offsets[key])
            tick_lab.append(label)
    ax.set_xticks(tick_pos)
    ax.set_xticklabels(tick_lab, fontsize=FONT_XTICK)
    ax.set_xlim(0.3, 5.2)

    ax.set_ylim(bottom=0.4, top=cap)
    ax.set_ylabel("Time (ms)", fontsize=FONT_PANEL)
    ax.set_title(title, fontsize=FONT_PANEL, pad=4)

    ax.yaxis.set_major_locator(MaxNLocator(nbins=7, min_n_ticks=7))
    ax.yaxis.set_major_formatter(FormatStrFormatter("%.1f"))
    ax.yaxis.set_minor_locator(AutoMinorLocator(2))
    ax.grid(True, axis="y", which="major", ls="--", alpha=0.25)
    ax.grid(True, axis="y", which="minor", ls=":",  alpha=0.12)
    ax.set_axisbelow(True)
    ax.set_box_aspect(SUBPLOT_H / SUBPLOT_W)

    return cap


# ── Main ─────────────────────────────────────────────────────────────────────
def none_or_str(v):
    return None if v.lower() == "none" else v


parser = argparse.ArgumentParser(
    description="1×2 violin (paper Fig 14) for E7 (or any E8-style folder).",
)
parser.add_argument("--gpu",        default="results/beverin",            metavar="DIR", type=none_or_str,
                    help="MI300A folder. Either E7-nested (results/beverin) or "
                         "flat (beverin_full_permutations_8). Default: results/beverin")
parser.add_argument("--gpu2",       default="results/daint",              metavar="DIR", type=none_or_str,
                    help="GH200 folder. Default: results/daint")
parser.add_argument("--gpu-title",  default="MI300A GPU",                 metavar="STR")
parser.add_argument("--gpu2-title", default="GH200 Hopper GPU",           metavar="STR")
parser.add_argument("--baseline",   default="unpermuted_lv0_sm0",         metavar="KEY",
                    help="Original-layout config (default for E7: unpermuted_lv0_sm0; "
                         "use 'unpermuted' for icon-artifacts/E8 layouts).")
parser.add_argument("--target",     default="nlev_first_lv0_sm1",         metavar="KEY",
                    help="Optimized-layout config (default for E7: nlev_first_lv0_sm1; "
                         "use 'nlev_first_shuffled' for icon-artifacts/E8 layouts).")
parser.add_argument("--out",        default="violin_stage8_gpu_cap.png", metavar="FILE",
                    help="output filename (default writes to cwd so the "
                         "Figures/plot_results.sh -maxdepth 1 finder picks it up).")
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

for folder, plat_title, col in PLATFORMS:
    ax = axes[col]
    if folder is None or not os.path.isdir(folder):
        ax.set_title(f"{plat_title}\n[no data]", fontsize=FONT_PANEL)
        continue
    plot_panel(ax, folder, plat_title, args.baseline, args.target,
               forced_cap=None, is_amd=(col == 0))

axes[1].set_ylabel("")

handles = [
    Patch(facecolor=COL_BASELINE,  edgecolor="black", label="Original Layout"),
    Patch(facecolor=COL_OPTIMIZED, edgecolor="black", label="Optimized Layout"),
]
fig.legend(handles=handles, loc="lower center",
           bbox_to_anchor=(0.5, -0.055), ncol=2, framealpha=0.9,
           fontsize=FONT_LEGEND)

fig.tight_layout(rect=[0, 0.08, 1, 0.82])
out_dir = os.path.dirname(args.out)
if out_dir:
    os.makedirs(out_dir, exist_ok=True)
fig.savefig(args.out, dpi=200, bbox_inches="tight")
fig.savefig(args.out.replace(".png", ".pdf"), bbox_inches="tight")
plt.close(fig)
print(f"Saved {args.out}")
