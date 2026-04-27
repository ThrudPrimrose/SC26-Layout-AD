#!/usr/bin/env python3
"""
plot_paper_v2.py -- E7 full velocity-tendencies, AD-layout violin plot.

Consumes the **new** results layout produced by ``run_{daint,beverin}.sh``:

    results/<platform>/<config>/ts<N>/run.txt

Each ``run.txt`` is the tee'd stdout/stderr from one binary invocation
(``velocity_stage5a_<config>`` or ``velocity_stage6_<config>``); the
per-rep timing comes from the host-side ``_insert_body_timer`` tasklet
inserted by ``utils.stages.stage4`` and prints exactly one line per
``__program_velocity_no_nproma_if_prop_*`` call:

    velocity body: 0.034567 ms

The first ``--warmup`` reps are still recorded (the timer fires on
every call); we drop them by default via ``--drop-first``.

Output: one PDF/PNG per timestep, paper-style violins, rows per
platform, columns per config-of-interest.

The original (pre-AD) ``plot_paper.py`` --- which expects the older
``<config>/<run.txt>`` layout used by the paper-snapshot CSVs --- is
preserved untouched. Use this script for reviewer-fresh runs.

Usage:
    python3 plot_paper_v2.py
    python3 plot_paper_v2.py --results-root results
    python3 plot_paper_v2.py --timesteps 7,9 --configs winner_v1_sm0,v123_cv_V6_ch_V6_f_V6_s_V6_n_V2_lm_V6
    python3 plot_paper_v2.py --drop-first 5      # drop warm-up reps already in the trace
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

# --- Shared plotting helpers (common/plot_util.py) --------------------
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

# Canonical paper styling (mirrors loopnest_*/plot_paper.py).
VIOLIN_BW_METHOD = "scott"
VIOLIN_POINTS = 200
SUBPLOT_W = 7.5
SUBPLOT_H = 3.5
VCOL = {"named": "#e67e22", "winner": "#2980b9"}
VLAB = {"named": "Named ablation", "winner": "Winner cross-product"}

_BODY_RE = re.compile(r"velocity body:\s+([0-9.eE+-]+)\s+ms")


def parse_run_txt(path):
    """Return all 'velocity body: <ms>' samples from a run.txt file."""
    out = []
    try:
        with open(path) as f:
            for line in f:
                m = _BODY_RE.search(line)
                if m:
                    try:
                        out.append(float(m.group(1)))
                    except ValueError:
                        pass
    except OSError:
        pass
    return out


def discover(results_root, platforms, timesteps_filter):
    """Return ``{(platform, config, ts): [ms_samples...]}``.

    Walks ``results_root/<platform>/<config>/ts<N>/run.txt``. Skips the
    Slurm log files that live directly under ``results_root/<platform>/``.
    """
    out = {}
    for plat in platforms:
        plat_dir = Path(results_root) / plat
        if not plat_dir.is_dir():
            continue
        for cfg_dir in sorted(plat_dir.iterdir()):
            if not cfg_dir.is_dir():
                continue
            cfg = cfg_dir.name
            for ts_dir in sorted(cfg_dir.iterdir()):
                if not ts_dir.is_dir() or not ts_dir.name.startswith("ts"):
                    continue
                try:
                    ts = int(ts_dir.name[2:])
                except ValueError:
                    continue
                if timesteps_filter and ts not in timesteps_filter:
                    continue
                run_txt = ts_dir / "run.txt"
                if not run_txt.is_file():
                    continue
                samples = parse_run_txt(run_txt)
                if samples:
                    out[(plat, cfg, ts)] = samples
    return out


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


def _classify(cname):
    """Return ('named', 'winner') based on the config name prefix."""
    return "winner" if cname.startswith("v123_") else "named"


def _select_configs(samples_by_key, ts, platforms, requested):
    """For the given timestep, return the ordered list of configs to
    plot. If ``requested`` is None, take the union across platforms."""
    if requested is not None:
        return list(requested)
    cfgs = set()
    for (plat, cfg, ts2), _ in samples_by_key.items():
        if ts2 == ts and plat in platforms:
            cfgs.add(cfg)
    # Stable order: named first (alphabetical), then v123 (alphabetical).
    named = sorted(c for c in cfgs if not c.startswith("v123_"))
    winner = sorted(c for c in cfgs if c.startswith("v123_"))
    return named + winner


def _short_label(cname):
    """Compact x-tick label: drop the v123_ prefix and group_ tags."""
    if not cname.startswith("v123_"):
        return cname
    # v123_cv_V6_ch_V1_f_V6_s_V6_n_V2_lm_V6 -> 6_1_6_6_2_6
    parts = cname[len("v123_"):].split("_")
    digits = []
    for tok in parts:
        if tok.startswith("V") and tok[1:].isdigit():
            digits.append(tok[1:])
    return "v" + "".join(digits) if digits else cname


def plot_one_timestep(samples_by_key, ts, configs, platforms, out_stem):
    nrows = sum(1 for p in platforms if any(
        (p, c, ts) in samples_by_key for c in configs))
    if nrows == 0:
        print(f"  [skip] ts={ts}: no platforms with samples"); return
    rows_present = [p for p in platforms
                    if any((p, c, ts) in samples_by_key for c in configs)]

    fig, axes = plt.subplots(
        len(rows_present), 1,
        figsize=(SUBPLOT_W, SUBPLOT_H * len(rows_present)),
        squeeze=False,
    )
    fig.suptitle(f"Velocity tendencies body time -- timestep {ts}",
                 fontsize=14, y=0.995)

    plt.rcParams.update({
        "font.size": 12, "axes.titlesize": 13, "axes.labelsize": 12,
        "xtick.labelsize": 9, "ytick.labelsize": 11, "legend.fontsize": 10,
    })

    for ri, plat in enumerate(rows_present):
        ax = axes[ri, 0]
        positions, data, colors = [], [], []
        med_annot = []
        x_labels = []

        for ci, cfg in enumerate(configs):
            samples = samples_by_key.get((plat, cfg, ts), [])
            x_labels.append(_short_label(cfg))
            if not samples:
                continue
            arr = np.asarray(samples, dtype=float)
            positions.append(ci)
            data.append(arr)
            colors.append(VCOL[_classify(cfg)])
            med = float(np.median(arr))
            lo, hi = bootstrap_ci(arr)
            med_annot.append((ci, med, lo, hi))

        if data:
            parts = ax.violinplot(
                data, positions=positions,
                showmeans=False, showmedians=True, showextrema=False,
                widths=0.85, bw_method=VIOLIN_BW_METHOD, points=VIOLIN_POINTS,
            )
            for body, c in zip(parts["bodies"], colors):
                body.set_facecolor(c)
                body.set_edgecolor("black")
                body.set_alpha(0.75)
            parts["cmedians"].set_color("white")

            ci_color = "#FF69B4"
            for pos, med, lo, hi in med_annot:
                if hi - lo < 1e-9:
                    continue
                ax.vlines(pos, lo, hi, color="black", lw=3, zorder=10)
                ax.vlines(pos, lo, hi, color=ci_color, lw=1.5, zorder=11)

        ax.set_xticks(range(len(configs)))
        ax.set_xticklabels(x_labels, rotation=45, ha="right")
        ax.set_ylabel("Body time [ms]")
        ax.yaxis.set_major_locator(MaxNLocator(nbins=6))
        ax.grid(axis="y", alpha=0.25)
        ax.set_axisbelow(True)
        ax.set_title(PLATFORM_LABEL.get(plat, plat), loc="left")

    handles = [Patch(facecolor=VCOL[k], edgecolor="black", label=VLAB[k])
               for k in ("named", "winner")]
    fig.legend(handles=handles, loc="lower center",
               bbox_to_anchor=(0.5, -0.02), ncol=2, framealpha=0.9)

    fig.tight_layout(rect=[0, 0.03, 1, 0.98])
    fig.savefig(f"{out_stem}.png", dpi=180, bbox_inches="tight")
    fig.savefig(f"{out_stem}.pdf", dpi=180, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved {out_stem}.png / .pdf  "
          f"(rows={rows_present}, configs={len(configs)})")


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--results-root", default="results", type=Path,
                    help="root containing <platform>/<config>/ts<N>/run.txt "
                         "(default: results)")
    ap.add_argument("--platforms", default="beverin,daint",
                    help="comma list of platforms to scan (default: beverin,daint)")
    ap.add_argument("--timesteps", default="",
                    help="comma list of timesteps to plot (default: every "
                         "ts<N>/ found on disk)")
    ap.add_argument("--configs", default="",
                    help="comma list of config names to include (default: "
                         "every config found in results/)")
    ap.add_argument("--drop-first", type=int, default=0,
                    help="drop the first N samples per (platform, config, ts) "
                         "as warm-up (default: 0; the binary's --warmup loop "
                         "already runs untimed, but the body timer fires on "
                         "every call so reviewers may want to drop a few more)")
    ap.add_argument("--out-stem", default="violins_velocity_body",
                    help="output filename stem (one file per timestep is "
                         "appended _ts<N>) (default: violins_velocity_body)")
    args = ap.parse_args()

    plats = [p.strip() for p in args.platforms.split(",") if p.strip()]
    ts_filter = ({int(t.strip()) for t in args.timesteps.split(",") if t.strip()}
                 if args.timesteps else None)
    cfg_request = ([c.strip() for c in args.configs.split(",") if c.strip()]
                   if args.configs else None)

    samples_by_key = discover(args.results_root, plats, ts_filter)
    if not samples_by_key:
        print(f"  [WARN] no run.txt files with 'velocity body:' lines found under "
              f"{args.results_root}")
        return 1

    if args.drop_first > 0:
        samples_by_key = {
            k: v[args.drop_first:] for k, v in samples_by_key.items()
            if len(v) > args.drop_first
        }

    timesteps_present = sorted({ts for (_, _, ts) in samples_by_key})
    print(f"plot_paper_v2: results_root={args.results_root}  platforms={plats}")
    print(f"  timesteps: {timesteps_present}")
    print(f"  total (platform, config, ts) keys: {len(samples_by_key)}")

    for ts in timesteps_present:
        configs = _select_configs(samples_by_key, ts, plats, cfg_request)
        if not configs:
            continue
        out_stem = f"{args.out_stem}_ts{ts}"
        plot_one_timestep(samples_by_key, ts, configs, plats, out_stem)

    return 0


if __name__ == "__main__":
    sys.exit(main())
