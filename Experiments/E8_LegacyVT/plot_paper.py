#!/usr/bin/env python3
"""
plot_paper.py -- E8 full velocity-tendencies (LEGACY pipeline) violins.

Source-of-truth driver for Figure 14 / Table V (paper §IV-D).

Consumes the **native** E8 result layout produced by
``run_stage8_permutations.py``:

    {daint,beverin}_full_permutations_8/<config>_<shuffled|unshuffled>.txt

Each ``.txt`` is the full stdout of one binary invocation; per-rep
timer lines come in two known shapes (we accept both, normalised to ms):

    Timer velocity_no_nproma_if_prop_lvn_only_<L>_istep_<I> took <us> us
    [Timer] Elapsed time: <ms> ms

Falls back to the new (E7-style) ``results/<platform>/<config>/ts<N>/run.txt``
layout when the native dir is empty -- so reviewers running the E7 sbatch
or local copies of the new naming convention still get plotted with the
same script.

Output: one PDF/PNG per (platform, dist) we have data for, paper-styled
violins. Names match the E6 loopnest plot_paper convention so
``Figures/plot_{results,paper_snapshot}.sh`` finds them by glob.

Usage:
    python plot_paper.py
    python plot_paper.py --root .
    python plot_paper.py --configs winner_v1,winner_v6
"""

import argparse
import os
import re
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from matplotlib.ticker import MaxNLocator
import numpy as np

try:
    from scipy.stats import bootstrap
    HAVE_SCIPY = True
except ImportError:
    HAVE_SCIPY = False

# --- Shared plotting helpers (common/plot_util.py) -----------------------
_HERE = Path(__file__).resolve().parent
_d = _HERE
while _d.name != "Experiments" and _d.parent != _d:
    _d = _d.parent
if _d.name == "Experiments":
    sys.path.insert(0, str(_d / "common"))
try:
    from plot_util import load_stream_peaks as _load_stream_peaks
    STREAM_PEAK = _load_stream_peaks()
except Exception:
    STREAM_PEAK = {}

PLATFORMS = ("beverin", "daint")
PLATFORM_LABEL = {"beverin": "MI300A", "daint": "GH200 Hopper"}

# Canonical paper styling.
VIOLIN_BW_METHOD = "scott"
VIOLIN_POINTS = 200
SUBPLOT_W = 7.5
SUBPLOT_H = 3.5

# Colour scheme: shuffled = orange (the §IV-D Original/winner_v1 baseline),
# unshuffled = blue (the §IV-D Optimized/winner_v6 layout); kept consistent
# across every E6/E7/E8 violin in the paper.
VCOL = {"shuffled": "#e67e22", "unshuffled": "#2980b9"}
VLAB = {"shuffled": "Shuffled (map shuffle on)",
        "unshuffled": "Unshuffled (map shuffle off)"}

# Two timer-line patterns DaCe + icon-artifacts emit. Both reduce to ms.
_TIMER_US_RE = re.compile(
    r"Timer\s+velocity_no_nproma_if_prop_lvn_only_(\d+)_istep_(\d+)\s+took\s+(\d+)\s+us"
)
_TIMER_MS_RE = re.compile(r"\[Timer\]\s+Elapsed\s+time:\s+([0-9.eE+-]+)\s+ms")
_BODY_MS_RE = re.compile(r"velocity body:\s+([0-9.eE+-]+)\s+ms")


def parse_timer_lines(path):
    """Return all per-rep elapsed-time samples from one log file in ms."""
    out = []
    try:
        with open(path) as f:
            for line in f:
                m = _TIMER_US_RE.search(line)
                if m:
                    out.append(int(m.group(3)) * 1e-3)
                    continue
                m = _TIMER_MS_RE.search(line)
                if m:
                    try:
                        out.append(float(m.group(1)))
                    except ValueError:
                        pass
                    continue
                m = _BODY_MS_RE.search(line)
                if m:
                    try:
                        out.append(float(m.group(1)))
                    except ValueError:
                        pass
    except OSError:
        pass
    return out


def discover(root, platforms, config_filter=None):
    """Scan E8's native + E7's per-timestep layouts. Returns
    ``{(platform, config, variant): [ms samples]}`` where ``variant`` is
    ``shuffled`` / ``unshuffled`` for the native E8 layout, or ``ts<N>``
    for the E7-style layout (so reviewers running either flow get plotted)."""
    out = defaultdict(list)

    for plat in platforms:
        # Native E8 layout: {plat}_full_permutations_8/<cfg>_<shuffled|unshuffled>.txt
        native_dir = root / f"{plat}_full_permutations_8"
        if native_dir.is_dir():
            for txt in sorted(native_dir.glob("*_shuffled.txt")) + sorted(
                native_dir.glob("*_unshuffled.txt")
            ):
                cfg, variant = txt.stem.rsplit("_", 1)
                if config_filter and cfg not in config_filter:
                    continue
                samples = parse_timer_lines(txt)
                if samples:
                    out[(plat, cfg, variant)].extend(samples)

        # E7-style fallback / additional layout: results/<plat>/<cfg>/ts<N>/run.txt
        plat_results = root / "results" / plat
        if plat_results.is_dir():
            for cfg_dir in sorted(plat_results.iterdir()):
                if not cfg_dir.is_dir():
                    continue
                cfg = cfg_dir.name
                if config_filter and cfg not in config_filter:
                    continue
                for ts_dir in sorted(cfg_dir.glob("ts*")):
                    if not ts_dir.is_dir():
                        continue
                    run_txt = ts_dir / "run.txt"
                    if not run_txt.is_file():
                        continue
                    variant = ts_dir.name  # ts7, ts9, ...
                    samples = parse_timer_lines(run_txt)
                    if samples:
                        out[(plat, cfg, variant)].extend(samples)

    return dict(out)


def bootstrap_ci(arr, confidence_level=0.95, n_resamples=10000):
    if len(arr) < 3 or not HAVE_SCIPY:
        med = float(np.median(arr))
        return med, med
    try:
        res = bootstrap((arr,), np.median,
                        confidence_level=confidence_level,
                        n_resamples=n_resamples, method="BCa")
        lo, hi = res.confidence_interval.low, res.confidence_interval.high
        if np.isnan(lo) or np.isnan(hi):
            raise ValueError
        return float(lo), float(hi)
    except Exception:
        med = float(np.median(arr))
        return med, med


def _classify(variant):
    """Map a variant tag to a colour-key. ts<N> tags fall under
    'unshuffled' so the E7-fallback layout still gets plotted with a
    consistent colour."""
    if variant in ("shuffled", "unshuffled"):
        return variant
    return "unshuffled"


def _short_label(cfg):
    """Compact x-tick label for the long v123_/cv*/etc names."""
    if cfg.startswith("v123_"):
        digits = [tok[1:] for tok in cfg[len("v123_"):].split("_")
                  if tok.startswith("V") and tok[1:].isdigit()]
        return "v" + "".join(digits) if digits else cfg
    return cfg


def plot_one_platform(samples_by_key, plat, configs, out_stem):
    """Emit one figure per platform with violins per (config, variant)."""
    keys_for_plat = [(c, v) for (p, c, v) in samples_by_key if p == plat]
    if not keys_for_plat:
        print(f"  [skip] {plat}: no data"); return
    variants = sorted({v for _, v in keys_for_plat},
                      key=lambda v: (v != "unshuffled", v != "shuffled", v))
    if not variants:
        return

    fig, ax = plt.subplots(figsize=(SUBPLOT_W, SUBPLOT_H))
    fig.suptitle(f"E8 velocity tendencies -- {PLATFORM_LABEL.get(plat, plat)}",
                 fontsize=14, y=0.995)
    plt.rcParams.update({
        "font.size": 12, "axes.titlesize": 13, "axes.labelsize": 12,
        "xtick.labelsize": 9, "ytick.labelsize": 11, "legend.fontsize": 10,
    })

    n_v = len(variants)
    width = 0.8 / max(1, n_v)
    positions, data, colors_, med_annot = [], [], [], []
    x_ticks, x_labels = [], []
    for ci, cfg in enumerate(configs):
        x_ticks.append(ci)
        x_labels.append(_short_label(cfg))
        for vi, variant in enumerate(variants):
            samples = samples_by_key.get((plat, cfg, variant), [])
            if not samples:
                continue
            arr = np.asarray(samples, dtype=float)
            pos = ci - 0.4 + (vi + 0.5) * width
            positions.append(pos)
            data.append(arr)
            colors_.append(VCOL.get(_classify(variant), "#888888"))
            lo, hi = bootstrap_ci(arr)
            med_annot.append((pos, float(np.median(arr)), lo, hi))

    if data:
        parts = ax.violinplot(
            data, positions=positions,
            showmeans=False, showmedians=True, showextrema=False,
            widths=width * 0.9, bw_method=VIOLIN_BW_METHOD, points=VIOLIN_POINTS,
        )
        for body, c in zip(parts["bodies"], colors_):
            body.set_facecolor(c)
            body.set_edgecolor("black")
            body.set_alpha(0.75)
        parts["cmedians"].set_color("white")

        ci_color = "#FF69B4"
        for pos, _, lo, hi in med_annot:
            if hi - lo < 1e-9:
                continue
            ax.vlines(pos, lo, hi, color="black", lw=3, zorder=10)
            ax.vlines(pos, lo, hi, color=ci_color, lw=1.5, zorder=11)

    ax.set_xticks(x_ticks)
    ax.set_xticklabels(x_labels, rotation=45, ha="right")
    ax.set_ylabel("Body time [ms]")
    ax.yaxis.set_major_locator(MaxNLocator(nbins=6))
    ax.grid(axis="y", alpha=0.25)
    ax.set_axisbelow(True)

    handles = []
    for v in variants:
        key = _classify(v)
        handles.append(Patch(facecolor=VCOL.get(key, "#888888"),
                             edgecolor="black",
                             label=VLAB.get(key, v)))
    if handles:
        ax.legend(handles=handles, loc="upper right", framealpha=0.9)

    fig.tight_layout(rect=[0, 0.0, 1, 0.97])
    out_pdf = f"{out_stem}_{plat}.pdf"
    out_png = f"{out_stem}_{plat}.png"
    fig.savefig(out_png, dpi=180, bbox_inches="tight")
    fig.savefig(out_pdf, dpi=180, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved {out_pdf} / .png  (configs={len(configs)}, variants={variants})")


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--root", type=Path, default=Path("."),
                    help="experiment root (default: .). Holds "
                         "{daint,beverin}_full_permutations_8/ and/or results/.")
    ap.add_argument("--platforms", default="beverin,daint",
                    help="comma list of platforms to scan")
    ap.add_argument("--configs", default="",
                    help="comma list of config names to include (default: every config found)")
    ap.add_argument("--out-stem", default="violins_velocity_e8",
                    help="output filename stem; per-platform files become "
                         "<stem>_<platform>.{png,pdf}")
    args = ap.parse_args()

    plats = [p.strip() for p in args.platforms.split(",") if p.strip()]
    cfg_filter = ([c.strip() for c in args.configs.split(",") if c.strip()]
                  if args.configs else None)

    samples_by_key = discover(args.root, plats, cfg_filter)
    if not samples_by_key:
        print(f"  [WARN] no timer samples found under {args.root} for platforms={plats}")
        return 1

    # Order configs deterministically: shorter named configs first
    # (winner_*, nlev_first, etc), then v123_/cv*/cohort_/etc.
    configs_present = sorted({c for (_, c, _) in samples_by_key},
                             key=lambda c: (c.startswith("v123_"),
                                            c.startswith("cv"),
                                            c.startswith("nest_"),
                                            c))
    if cfg_filter:
        configs_present = [c for c in configs_present if c in cfg_filter]

    print(f"plot_paper (E8): root={args.root}  platforms={plats}")
    print(f"  configs:    {len(configs_present)}")
    print(f"  total keys: {len(samples_by_key)}")

    for plat in plats:
        plot_one_platform(samples_by_key, plat, configs_present, args.out_stem)

    return 0


if __name__ == "__main__":
    sys.exit(main())
