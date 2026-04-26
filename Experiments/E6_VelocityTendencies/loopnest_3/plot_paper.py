#!/usr/bin/env python3
"""
plot_paper.py — 2×2 violin bandwidth plot for one E6 loopnest

Rows: platforms (AMD beverin, NVIDIA daint)
Cols: device (CPU, GPU)

Median of each violin is annotated as % of the platform's STREAM peak.
Only the `KERNEL` constant below changes across loopnest_1..6; the
rest of the file is identical so plots stay visually consistent.

Orange = Horizontal-first (V1)            — best config / schedule
Blue   = Best-performing vertical-first   — best over V2..V5
"""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from matplotlib.ticker import MaxNLocator
import pandas as pd, numpy as np, argparse

# ══════════════════════════════════════════════════════════════════════
#  Per-loopnest configuration (the ONLY line each loopnest overrides)
# ══════════════════════════════════════════════════════════════════════
KERNEL = "z_v_grad_w_full"   # loopnest_1: indirect stencil

# ---- CSV paths derived from KERNEL + cluster -----------------------
GPU_AMD_CSV = f"results/beverin/{KERNEL}_gpu.csv"
CPU_AMD_CSV = f"results/beverin/{KERNEL}_cpu.csv"
GPU_NV_CSV  = f"results/daint/{KERNEL}_gpu.csv"
CPU_NV_CSV  = f"results/daint/{KERNEL}_cpu.csv"

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

SUBPLOT_W = 5.5   # wider to fit 3 distributions
SUBPLOT_H = 3.5   # inches per row

N_e  = 122880   # valid edges (loop bound)
N_c  = 81920    # cells (indirect target for w)
N_v  = 81920    # vertices (indirect target for z_w_v)
NLEV = 96
DISTS    = ["uniform", "normal_var1", "exact"]
OUT_STEM = f"violins_{KERNEL}_nlev{NLEV}"

BYTES = (
    2 * N_e * 4 +           # cell_idx    [N_e × 2]        int read
    2 * N_e * 4 +           # vert_idx    [N_e × 2]        int read
    NLEV * N_e * 8 +        # vn_ie       [N_e × nlev]     dbl read
    NLEV * N_c * 8 +        # w           [N_c × nlev]     dbl read (indirect)
    N_e * 8 +               # inv_dual    [N_e]            dbl read
    NLEV * N_e * 8 +        # z_vt_ie     [N_e × nlev]     dbl read
    NLEV * N_v * 8 +        # z_w_v       [N_v × nlev]     dbl read (indirect)
    N_e * 8 +               # inv_primal  [N_e]            dbl read
    N_e * 8 +               # tangent     [N_e]            dbl read
    NLEV * N_e * 8           # out         [N_e × nlev]     dbl write
)

GRID = [
    [("MI300A Zen Cores", CPU_AMD_CSV, "parallelization", None,        "cpu_scalar"),
     ("MI300A",           GPU_AMD_CSV, "config_label",    "1x1_32x16", "gpu_scalar")],
    [("Grace Neoverse v2", CPU_NV_CSV, "parallelization", None,        "cpu_scalar"),
     ("H200",              GPU_NV_CSV, "config_label",    "1x1_32x16", "gpu_scalar")],
]

VCOL = {"v1": "#e67e22", "best": "#2980b9"}
VLAB = {"v1": "Horizontal-first Layout", "best": "Best Vertical-first Layout"}
DIST_LABEL = {
    "uniform":     "Uniform",
    "normal_var1": r"Normal ($\sigma^2\!=\!1$)",
    "exact":       "R02B05",
}

parser = argparse.ArgumentParser()
parser.add_argument('--add-peak', action='store_true', default=True)
args = parser.parse_args()

def compute_bandwidth(df):
    return BYTES / (df["time_ms"] * 1e-3) / 1e12


# ---------------------------------------------------------------------------
#  Data-selection helpers
# ---------------------------------------------------------------------------

def _best_group(sub, group_col):
    """Return (best_key, filtered_sub) choosing the group with highest median BW."""
    sub = sub.copy()
    sub["bandwidth"] = compute_bandwidth(sub)
    medians = sub.groupby(group_col)["bandwidth"].median()
    if medians.empty:
        return None, sub.iloc[0:0]
    best = medians.idxmax()
    return best, sub[sub[group_col] == best]


def get_v1_best_data(df_full, fcol, dist, nlev):
    """
    Orange violin: always variant 1, but pick the best config/schedule.
    """
    V = 1
    sub = df_full[(df_full["variant"] == V) &
                  (df_full["cell_dist"] == dist) &
                  (df_full["nlev"] == nlev)]
    if sub.empty:
        return np.array([])
    best_key, best_df = _best_group(sub, fcol)
    return best_df["bandwidth"].values if not best_df.empty else np.array([])


def get_overall_best_data(df_full, fcol, dist, nlev):
    """
    Blue violin: pick whichever (variant, config/schedule) pair gives the
    highest median bandwidth — completely unconstrained.
    """
    sub = df_full[(df_full["variant"].isin([3, 4, 5, 6, 7])) &
                  (df_full["cell_dist"] == dist) &
                  (df_full["nlev"] == nlev)]
    if sub.empty:
        return np.array([])
    sub = sub.copy()
    sub["bandwidth"] = compute_bandwidth(sub)
    medians = sub.groupby(["variant", fcol])["bandwidth"].median()
    if medians.empty:
        return np.array([])
    best_var, best_cfg = medians.idxmax()
    mask = (sub["variant"] == best_var) & (sub[fcol] == best_cfg)
    return sub.loc[mask, "bandwidth"].values


# ---------------------------------------------------------------------------
#  Main
# ---------------------------------------------------------------------------

def _normalise_schema(df: pd.DataFrame) -> pd.DataFrame:
    """Accept both the L1 and L2-6 CSV schemas.

    L1 uses `variant, parallelization, N_c, N_v`; L2-6 use `V, schedule`
    and no `N_c`/`N_v`. Rename the L2-6 columns to the L1 names so the
    rest of this script works unchanged across all six loopnests.
    """
    renames = {}
    if "variant" not in df.columns and "V" in df.columns:
        renames["V"] = "variant"
    if "parallelization" not in df.columns and "schedule" in df.columns:
        renames["schedule"] = "parallelization"
    if renames:
        df = df.rename(columns=renames)
    return df


def main():
    raw = {}
    for row in GRID:
        for label, csv, fcol, fval, _ in row:
            if label not in raw:
                try:
                    df = _normalise_schema(pd.read_csv(csv))
                except FileNotFoundError:
                    # Missing panel -> leave it blank, don't abort the figure.
                    print(f"  [WARN] {csv} not found (panel will be empty)")
                    continue
                raw[label] = (df, fcol, fval)
    if not raw:
        print("  [WARN] no CSVs loaded; nothing to plot"); return

    plt.rcParams.update({
        "font.size": 14, "axes.titlesize": 15, "axes.labelsize": 14,
        "xtick.labelsize": 12, "ytick.labelsize": 12, "legend.fontsize": 12,
    })

    nrows = len(GRID)
    ncols = max(len(row) for row in GRID)

    fig, axes = plt.subplots(
        nrows, ncols,
        figsize=(SUBPLOT_W * ncols, SUBPLOT_H * nrows),
        squeeze=False,
    )
    fig.suptitle("Semi-Structured Weather Stencil Pattern from ICON",
                fontsize=15, y=0.975)
    fig.text(0.5, 0.935, "% annotations relative to STREAM peak bandwidth",
            ha='center', va='top', fontsize=12, color='dimgray')

    ndists = len(DISTS)
    group_spacing = 3  # positions per distribution group (2 violins + 1 gap)

    violin_keys = ["v1", "best"]   # orange, blue

    for ri, row_data in enumerate(GRID):
        for ci, (label, csv, fcol, fval, _) in enumerate(row_data):
            ax = axes[ri, ci]
            if label not in raw:
                # CSV missing: show an empty placeholder instead of crashing.
                ax.text(0.5, 0.5, f"(no {label} data)",
                        ha='center', va='center', fontsize=12, color='gray',
                        transform=ax.transAxes)
                ax.set_xticks([]); ax.set_yticks([])
                ax.set_title(label, loc='left')
                continue
            df_full = raw[label][0]
            positions, data_all, col_all = [], [], []
            medians_for_pct = []   # (pos, median_bw, key)
            xticks, xlabels = [], []

            # ── Collect data ──────────────────────────────────────────────
            for di, dist in enumerate(DISTS):
                for vi, vk in enumerate(violin_keys):
                    if vk == "v1":
                        vals = get_v1_best_data(df_full, fcol, dist, NLEV)
                    else:
                        vals = get_overall_best_data(df_full, fcol, dist, NLEV)

                    # No outlier trimming (repo-wide policy).
                    pos = di * group_spacing + vi
                    if len(vals) > 0:
                        data_all.append(vals)
                        positions.append(pos)
                        col_all.append(VCOL[vk])
                        medians_for_pct.append((pos, np.median(vals), vk))
                xticks.append(di * group_spacing + 0.5)
                xlabels.append(DIST_LABEL[dist])

            # ── Compute y-axis limits & 6 evenly-spaced ticks ─────────────
            all_vals_flat = np.concatenate(data_all) if data_all else np.array([0.0])
            max_val = float(np.max(all_vals_flat))

            locator = MaxNLocator(nbins=5, min_n_ticks=5)
            candidate_ticks = locator.tick_values(0.0, max_val * 1.14)
            candidate_ticks = candidate_ticks[candidate_ticks >= 0]
            if len(candidate_ticks) > 6:
                candidate_ticks = candidate_ticks[:6]

            top_lim = candidate_ticks[-1] * 1.06 if len(candidate_ticks) else max_val * 1.15

            # ── Plot violins (canonical sampling: bw=scott, pts=200) ──────
            if data_all:
                parts = ax.violinplot(data_all, positions=positions,
                                      showmeans=True, showmedians=True,
                                      showextrema=False, widths=1.2,
                                      bw_method="scott", points=200)
                for i, body in enumerate(parts["bodies"]):
                    body.set_facecolor(col_all[i])
                    body.set_edgecolor("black")
                    body.set_alpha(0.75)
                parts["cmeans"].set_color("black")
                parts["cmedians"].set_color("white")

            # ── Axes decoration ───────────────────────────────────────────
            ax.set_xticks(xticks)
            ax.set_xticklabels(xlabels)
            ax.set_yticks(candidate_ticks)
            ax.set_ylim(bottom=0, top=top_lim)

            # Bandwidth label only on left column
            if ci == 0:
                ax.set_ylabel("Bandwidth [TB/s]")

            ax.set_title(label)
            ax.grid(axis="y", alpha=0.3)

            if args.add_peak and label in STREAM_PEAK:
                peak = STREAM_PEAK[label]
                ax.axhline(y=peak, color='red', linestyle='--', linewidth=1.5, alpha=0.7)
                x_min, x_max = ax.get_xlim()
                ax.text(x_max - 0.02 * (x_max - x_min), peak * 0.98,
                        f'STREAM Peak: {peak:.1f} TB/s',
                        ha='right', va='top', fontsize=9, color='red')

            # ── STREAM peak label — top-left corner of each subplot ───────
            if label in STREAM_PEAK:
                peak_gb = round(STREAM_PEAK[label] * 1e3)   # convert TB/s → GB/s integer
                ax.text(0.03, 0.97, f"{peak_gb*1e-3:.2f} TB/s STREAM Peak",
                        transform=ax.transAxes,
                        ha='left', va='top',
                        fontsize=10, color='dimgray')

            # ── Per-violin % of STREAM peak annotation ────────────────────
            if label in STREAM_PEAK:
                peak = STREAM_PEAK[label]
                ylo, yhi = ax.get_ylim()
                y_range = yhi - ylo
                offset_down = 0.045 * y_range

                for pos, med_bw, vk in medians_for_pct:
                    pct = 100.0 * med_bw / peak
                    color = VCOL[vk]
                    if vk == "v1":   # orange → bottom-left of violin
                        xoff, ha = -0.1, 'right'
                    else:            # blue  → bottom-right of violin
                        xoff, ha = +0.1, 'left'
                    ax.text(pos + xoff, med_bw - offset_down,
                            f'{pct:.0f}%',
                            ha=ha, va='top',
                            fontsize=12, color=color, fontweight='bold')

    # ── Legend ────────────────────────────────────────────────────────────
    v_handles = [
        Patch(facecolor=VCOL["v1"],   edgecolor="black", label=VLAB["v1"]),
        Patch(facecolor=VCOL["best"], edgecolor="black", label=VLAB["best"]),
    ]
    fig.legend(handles=v_handles, loc='lower center',
               bbox_to_anchor=(0.5, -0.008), ncol=2, framealpha=0.9,
               columnspacing=1.0)

    fig.tight_layout(rect=[0, 0.04, 1, 0.999])
    sfx = "_w_stream_peak" if args.add_peak else ""
    fig.savefig(f"{OUT_STEM}{sfx}.png", dpi=180, bbox_inches='tight')
    fig.savefig(f"{OUT_STEM}{sfx}.pdf", dpi=180, bbox_inches='tight')

    # ── Summary table ─────────────────────────────────────────────────────
    print("\nBest schedule/config selection:")
    print(f"{'Platform':<22} {'Violin':<10} {'Distribution':<20} {'Best (Var, Cfg)':<30} {'Median TB/s':<12} {'Runtime ms/s':<12} {'% Peak':<8}")
    print("-" * 102)
    for row_data in GRID:
        for label, csv, fcol, fval, _ in row_data:
            if label not in raw:
                continue   # CSV was missing -- no summary row for this panel.
            df_full = raw[label][0]
            for dist in DISTS:
                # --- orange (V1 best) ---
                sub_v1 = df_full[(df_full["variant"] == 1) &
                                 (df_full["cell_dist"] == dist) &
                                 (df_full["nlev"] == NLEV)]
                if not sub_v1.empty:
                    sub_v1 = sub_v1.copy()
                    sub_v1["bandwidth"] = compute_bandwidth(sub_v1)
                    meds = sub_v1.groupby(fcol)["bandwidth"].median()
                    meds2 = sub_v1.groupby(fcol)["time_ms"].median()
                    if not meds.empty:
                        best_cfg = meds.idxmax()
                        med = meds.max()
                        pct = 100.0 * med / STREAM_PEAK.get(label, 1.0)
                        ms = meds2.min()
                        print(f"{label:<22} {'orange':<10} {dist:<20} {'V1, ' + str(best_cfg):<30} {med:.4f}  {ms:.4f}    {pct:.1f}%")

                # --- blue (overall best) ---
                sub_all = df_full[(df_full["variant"].isin([3, 4, 5, 6, 7])) &
                                  (df_full["cell_dist"] == dist) &
                                  (df_full["nlev"] == NLEV)]
                if not sub_all.empty:
                    sub_all = sub_all.copy()
                    sub_all["bandwidth"] = compute_bandwidth(sub_all)
                    meds = sub_all.groupby(["variant", fcol])["bandwidth"].median()
                    meds2 = sub_all.groupby(["variant", fcol])["time_ms"].median()
                    if not meds.empty:
                        best_var, best_cfg = meds.idxmax()
                        med = meds.max()
                        ms = meds2.min()
                        pct = 100.0 * med / STREAM_PEAK.get(label, 1.0)
                        print(f"{label:<22} {'blue':<10} {dist:<20} {'V' + str(best_var) + ', ' + str(best_cfg):<30} {med:.4f}   {ms:.4f}   {pct:.1f}%")

    print(f"\nSaved: {OUT_STEM}{sfx}.png, {OUT_STEM}{sfx}.pdf")

if __name__ == "__main__":
    main()
