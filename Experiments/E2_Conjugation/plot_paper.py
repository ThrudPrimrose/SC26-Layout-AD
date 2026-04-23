#!/usr/bin/env python3
"""
plot_conjugate.py – Line+scatter plots for the conjugate microbenchmark.

2×2 grid:  rows = {AMD MI300A, NVIDIA GH200},  cols = {CPU, GPU}
X-axis:    P (number of complex pairs) = 3, 6, 9, 12, 15, 18, 21
Lines:     AoS, SoA, AoSoA-16
Markers:   median ± 95% CI,  shaded band = CI

Generates two figures: one for in-place, one for out-of-place.

Usage:
    python plot_conjugate.py
    python plot_conjugate.py --bev-dir results/beverin --daint-dir results/daint
"""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import AutoMinorLocator, MaxNLocator, FormatStrFormatter
import numpy as np
import argparse, os, sys
from collections import defaultdict

# ══════════════════════════════════════════════════════════════════════
#  Constants
# ══════════════════════════════════════════════════════════════════════

# --- Shared plotting helpers (common/plot_util.py) --------------------
import os as _os, sys as _sys
_here = _os.path.dirname(_os.path.abspath(__file__))
_d = _here
while _os.path.basename(_d) != "Experiments" and _os.path.dirname(_d) != _d:
    _d = _os.path.dirname(_d)
if _os.path.basename(_d) == "Experiments":
    _sys.path.insert(0, _os.path.join(_d, "common"))
from plot_util import load_stream_peaks as _load_stream_peaks

STREAM_PEAK = _load_stream_peaks()

LAYOUTS = ["AoS", "SoA", "AoSoA-16"]

STYLE = {
    "AoS":      dict(color="#e74c3c", marker="o",  ls="-",  lw=1.0),
    "SoA":      dict(color="#2ecc71", marker="s",  ls="-",  lw=1.0),
    "AoSoA-16": dict(color="#9b59b6", marker="D",  ls="--", lw=1.0),
}

# ══════════════════════════════════════════════════════════════════════
#  CSV parsing
# ══════════════════════════════════════════════════════════════════════

def parse_csv(path):
    """Return {(P, layout): [gbps, …]}."""
    groups = defaultdict(list)
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("P,"): continue
            parts = line.split(",")
            if len(parts) < 5: continue
            try:
                P      = int(parts[0])
                layout = parts[1].strip()
                gbps   = float(parts[4]) * 1e-3  # GB/s → TB/s
                groups[(P, layout)].append(gbps)
            except (ValueError, IndexError):
                continue
    return groups

# ══════════════════════════════════════════════════════════════════════
#  Statistics
# ══════════════════════════════════════════════════════════════════════

def stats(vals):
    """Return (median, p5, p95)."""
    a = np.array(vals)
    if len(a) < 3:
        m = np.median(a)
        return m, m, m
    med = np.median(a)
    p5  = np.percentile(a, 5)
    p95 = np.percentile(a, 95)
    return med, p5, p95

# ══════════════════════════════════════════════════════════════════════
#  Panel drawing
# ══════════════════════════════════════════════════════════════════════

def draw_panel(ax, groups, title, peak=None, add_peak=True, gap_arrow_at=None):
    """Draw line+scatter+CI for one subplot."""

    ps_all = sorted({p for (p, _) in groups})
    if not ps_all:
        ax.set_title(title, fontsize=11)
        return

    ymax = 0

    for lay in LAYOUTS:
        ps, meds, lo, hi = [], [], [], []
        for p in ps_all:
            key = (p, lay)
            if key not in groups: continue
            m, cl, ch = stats(groups[key])
            ps.append(p)
            meds.append(m)
            lo.append(cl)
            hi.append(ch)
        if not ps: continue

        ps   = np.array(ps)
        meds = np.array(meds)
        lo   = np.array(lo)
        hi   = np.array(hi)
        ymax = max(ymax, float(np.max(hi)))

        sty = STYLE[lay]

        # Shaded P5–P95 band
        ax.fill_between(ps, lo, hi, color=sty["color"], alpha=0.22, edgecolor="none")

        # Line (median)
        ax.plot(ps, meds,
                color=sty["color"], ls=sty["ls"], lw=sty["lw"],
                marker=sty["marker"], markersize=6, markeredgecolor="white",
                markeredgewidth=0.8, label=lay, zorder=3)

        # Error bars (P5–P95)
        yerr = np.array([meds - lo, hi - meds])
        ax.errorbar(ps, meds, yerr=yerr, fmt="none",
                    ecolor=sty["color"], elinewidth=1.4, capsize=4,
                    capthick=1.2, alpha=0.8, zorder=2)

    # STREAM peak line
    if add_peak and peak:
        ax.axhline(y=peak, color="dimgray", ls="--", lw=1, alpha=0.5, zorder=1)
        ax.text(ps_all[0], peak * 1.02,
                f"STREAM {peak:.2f} TB/s",
                fontsize=9, color="dimgray", va="bottom", zorder=1)
        if peak > ymax:
            ymax = peak

    # ── Gap arrow at specific P between AoS and SoA ──
    if gap_arrow_at is not None:
        p_val = gap_arrow_at
        aos_key = (p_val, "AoS")
        soa_key = (p_val, "SoA")
        if aos_key in groups and soa_key in groups:
            y_aos = stats(groups[aos_key])[0] + 0.1
            y_soa = stats(groups[soa_key])[0] - 0.24
            arrow_x = p_val
            # Guide lines
            ax.plot([p_val, arrow_x], [y_aos, y_aos],
                    color="#555555", ls=":", lw=1.5, alpha=0.5, zorder=20)
            ax.plot([p_val, arrow_x], [y_soa, y_soa],
                    color="#555555", ls=":", lw=1.5, alpha=0.5, zorder=20)
            # Double-headed arrow
            ax.annotate("", xy=(arrow_x, y_soa), xytext=(arrow_x, y_aos),
                        arrowprops=dict(arrowstyle="<->", color="#555555",
                                        lw=1.5, shrinkA=0, shrinkB=0),
                        zorder=20)
            # Label
            mid_y = (y_aos + y_soa) / 2
            ax.text(arrow_x - 0.6, mid_y,
                    "Gap between\nbest schedule\nand best layout",
                    fontsize=9, color="#555555", va="center", ha="right",
                    style="italic", zorder=20)

    # Axes — force exactly 5 major y-ticks
    ax.set_xticks(ps_all)
    ax.set_xlim(ps_all[0] - 1, ps_all[-1] + 1)
    top = ymax * 1.12
    ax.set_ylim(0, top)
    ax.yaxis.set_major_locator(MaxNLocator(nbins=5, min_n_ticks=5))
    ax.yaxis.set_major_formatter(FormatStrFormatter("%.1f"))
    ax.yaxis.set_minor_locator(AutoMinorLocator(4))
    ax.tick_params(axis="y", which="minor", length=3)
    ax.grid(axis="y", alpha=0.25, zorder=0)
    ax.grid(axis="y", which="minor", alpha=0.12, ls=":", zorder=0)
    ax.grid(axis="x", alpha=0.12, ls=":", zorder=0)
    ax.set_axisbelow(True)
    ax.set_title(title, fontsize=11)

# ══════════════════════════════════════════════════════════════════════
#  Grid assembly
# ══════════════════════════════════════════════════════════════════════

def build_figure(grid, mode_label, out_stem):
    """
    grid: dict  (row_key, col_key) → (title, groups, peak)
    mode_label: "In-Place" or "Out-of-Place"
    """
    if not grid:
        print(f"  [{mode_label}] no data"); return

    active_rows = sorted({r for r, _ in grid}, key=lambda x: ["amd", "nv"].index(x))
    active_cols = sorted({c for _, c in grid}, key=lambda x: ["cpu", "gpu"].index(x))
    nrows = len(active_rows)
    ncols = len(active_cols)

    fig, axes = plt.subplots(nrows, ncols,
                             figsize=(3.6 * ncols, 2.8 * nrows + 1.1),
                             squeeze=False)

    for ri, rk in enumerate(active_rows):
        for ci, ck in enumerate(active_cols):
            ax = axes[ri, ci]
            ax.set_box_aspect(2.8 / 4.8)
            if (rk, ck) not in grid:
                ax.set_visible(False); continue
            title, groups, peak = grid[(rk, ck)]
            # Gap arrow on Nvidia GPU panel at P=21
            do_gap = 21 if (rk == "nv" and ck == "gpu") else None
            draw_panel(ax, groups, title, peak, gap_arrow_at=do_gap)
            if ci == 0:
                ax.set_ylabel("Bandwidth [TB/s]", fontsize=11)
            if ri == nrows - 1:
                ax.set_xlabel("Number of complex arrays (P)", fontsize=11)

    # Legend from bottom-right panel
    handles, labels = axes[-1, -1].get_legend_handles_labels()
    if not handles:
        for ax in axes.flat:
            handles, labels = ax.get_legend_handles_labels()
            if handles: break
    if handles:
        fig.legend(handles, labels,
                   loc="lower center", ncol=len(labels),
                   fontsize=10, frameon=True, fancybox=True,
                   bbox_to_anchor=(0.5, 0.02))

    titles = {
        "In-Place":      r"In-Place Conjugate: $A = \bar{A}$ for P Complex Arrays",
        "Out-of-Place":  r"Out-of-Place Conjugate: $B = \bar{A}$ P Complex Arrays",
    }
    fig.suptitle(
        f"{titles[mode_label]}",
        fontsize=15, y=0.89 if nrows > 1 else 0.98)
    fig.text(0.5, 0.85 if nrows > 1 else 0.95,
             "% annotations relative to STREAM peak bandwidth",
             ha='center', va='top', fontsize=12, color='dimgray')
    fig.tight_layout(rect=[0, 0.06, 1, 0.94 if nrows > 1 else 0.92],
                      h_pad=0.0)

    for ext in ("pdf", "png"):
        fig.savefig(f"{out_stem}.{ext}", dpi=200, bbox_inches="tight")
    print(f"  Saved {out_stem}.pdf / .png")
    plt.close(fig)

# ══════════════════════════════════════════════════════════════════════
#  Main
# ══════════════════════════════════════════════════════════════════════

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bev-dir",   default="results/beverin",
                    help="Directory with Beverin CSVs")
    ap.add_argument("--daint-dir", default="results/daint",
                    help="Directory with Daint CSVs")
    ap.add_argument("--out-prefix", default="conjugate", help="Output filename prefix")
    args = ap.parse_args()

    slots = [
        ("amd", "cpu", "MI300A Zen CPU",  "MI300A Zen CPU",  args.bev_dir,   "results_cpu"),
        ("amd", "gpu", "MI300A GPU",  "MI300A GPU",  args.bev_dir,   "results_gpu"),
        ("nv",  "cpu", "GH200 Grace CPU",   "GH200 Grace CPU",   args.daint_dir, "results_cpu"),
        ("nv",  "gpu", "GH200 Hopper GPU",   "GH200 Hopper GPU",  args.daint_dir, "results_gpu"),
    ]

    for mode, mode_label in [("oop", "Out-of-Place"), ("inplace", "In-Place")]:
        grid = {}
        for rk, ck, label, pk, dirpath, stem in slots:
            fname = f"{stem}_{mode}.csv"
            path  = os.path.join(dirpath, fname)
            if not os.path.isfile(path):
                path2 = os.path.join(dirpath, f"{stem}{mode}.csv")
                if os.path.isfile(path2):
                    path = path2
                else:
                    continue
            groups = parse_csv(path)
            if not groups: continue
            peak = STREAM_PEAK.get(pk, None)
            grid[(rk, ck)] = (label, groups, peak)

        # Sanity check: data_max > peak is possible because the recorded
        # STREAM peak comes from E0_NUMA's bench run, which cannot sweep
        # every platform-specific schedule/tile combination. Warn and
        # plot against the existing peak rather than aborting the figure.
        for (rk, ck), (label, groups, peak) in grid.items():
            if peak is None:
                continue
            all_vals = np.concatenate([np.array(v) for v in groups.values()])
            data_max = float(np.max(all_vals))
            if data_max > peak:
                print(
                    f"  [warn] {label} ({mode_label}): measured BW "
                    f"{data_max:.3f} TB/s > STREAM peak {peak:.3f} TB/s "
                    f"({(data_max/peak - 1) * 100:+.1f}%). Using existing "
                    f"peak; consider rerunning E0_NUMA STREAM to tighten "
                    f"the bound.",
                    file=_sys.stderr,
                )

        build_figure(grid, mode_label, f"{args.out_prefix}_{mode}")

    # Summary table
    for mode in ["oop", "inplace"]:
        print(f"\n{'='*60}")
        print(f"  {mode.upper()} summary (medians)")
        print(f"{'='*60}")
        for rk, ck, label, pk, dirpath, stem in slots:
            fname = f"{stem}_{mode}.csv"
            path  = os.path.join(dirpath, fname)
            if not os.path.isfile(path): continue
            groups = parse_csv(path)
            if not groups: continue
            peak = STREAM_PEAK.get(pk, 0)
            print(f"\n  {label}:")
            ps = sorted({p for p, _ in groups})
            header = f"    {'P':>3}  " + "  ".join(f"{l:>10}" for l in LAYOUTS)
            print(header)
            for p in ps:
                vals = []
                for lay in LAYOUTS:
                    key = (p, lay)
                    if key in groups:
                        m, _, _ = stats(groups[key])
                        pct = f"({100*m/peak:.0f}%)" if peak else ""
                        vals.append(f"{m:7.3f}{pct:>5}")
                    else:
                        vals.append(f"{'—':>12}")
                print(f"    {p:3d}  " + "  ".join(vals))

if __name__ == "__main__":
    main()