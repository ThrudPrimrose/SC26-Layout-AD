"""
plot_paper.py -- NumaStream bandwidth sweep visualization.

Reads results/{daint,beverin}/{numa_cpu,numa_gpu}.csv via the shared
helpers in common/plot_util.py (no hard-coded paths; results are
discovered relative to this file's location).

Emits:
  numa_cpu.pdf -- per-platform violin: distribution of bandwidth over
                  all reps, grouped by variant and matrix size.
  numa_gpu.pdf -- per-platform heatmap: median bandwidth as a function
                  of (block size, grid multiplier), one panel per N.

Repo-wide plotting policy (canonical_violin + no outlier trimming)
applies. Inputs are the tall, per-rep CSVs emitted by bench_cpu /
bench_gpu, so the violins see every collected sample.
"""
from __future__ import annotations

import os
import sys

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# common/plot_util.py is two directories up.
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "common"))
from plot_util import (  # noqa: E402
    PLATFORMS,
    canonical_violin,
    experiment_dir,
    fig_output_path,
    load_csv,
    remove_outliers,
)

EXP_DIR = experiment_dir(__file__)


# ── CPU: violin of bandwidth vs variant, grouped by N ─────────────────────
def plot_cpu(ax, df, platform):
    if df.empty:
        ax.text(0.5, 0.5, f"(no data: {platform})",
                ha="center", va="center", transform=ax.transAxes)
        ax.set_title(f"{platform.title()} CPU")
        ax.set_axis_off()
        return

    variants_pref = ["baseline_ft", "numa4_stripe", "numa3_stripe",
                     "numa2_stripe", "interleave"]
    vs = [v for v in variants_pref if v in set(df["variant"])]
    ns = sorted(df["N"].unique())

    datasets, positions, colors = [], [], []
    cmap = plt.cm.viridis(np.linspace(0.15, 0.85, len(vs)))
    width = 0.8 / max(len(vs), 1)
    for i, v in enumerate(vs):
        for k, n in enumerate(ns):
            sub = df[(df["variant"] == v) & (df["N"] == n)]
            arr = np.asarray(remove_outliers(sub["bw_gbs"].to_numpy()))
            if arr.size == 0:
                continue
            datasets.append(arr)
            positions.append(k + i * width - (len(vs) - 1) * width / 2)
            colors.append(cmap[i])

    if datasets:
        parts = canonical_violin(ax, datasets, positions, widths=width * 0.9)
        for body, c in zip(parts["bodies"], colors):
            body.set_facecolor(c)
            body.set_edgecolor("black")
            body.set_alpha(0.75)

    ax.set_xticks(range(len(ns)))
    ax.set_xticklabels([f"{int(n)}" for n in ns])
    ax.set_xlabel("Matrix edge N")
    ax.set_ylabel("Bandwidth [GB/s]")
    ax.set_title(f"{platform.title()} CPU  --  C = alpha * (A + B)")
    ax.grid(axis="y", linestyle=":", alpha=0.5)

    handles = [plt.Rectangle((0, 0), 1, 1, facecolor=c, edgecolor="black",
                             alpha=0.75) for c in cmap[:len(vs)]]
    ax.legend(handles, vs, loc="best", fontsize=8)


# ── GPU: median bandwidth heatmap, one panel per N ────────────────────────
def plot_gpu_heatmaps(fig, axes_row, df, platform):
    if df.empty:
        for ax in axes_row:
            ax.text(0.5, 0.5, f"(no data: {platform})",
                    ha="center", va="center", transform=ax.transAxes)
            ax.set_axis_off()
        return

    ns = sorted(df["N"].unique())
    blocks = sorted(df["block"].unique())
    mults = sorted(df["grid_mult"].unique())

    for panel, n in enumerate(ns):
        if panel >= len(axes_row):
            break
        ax = axes_row[panel]
        sub = df[df["N"] == n]
        grid = (sub.groupby(["block", "grid_mult"])["bw_gbs"]
                   .median()
                   .unstack("grid_mult"))
        grid = grid.reindex(index=blocks, columns=mults)

        im = ax.imshow(grid.values, origin="lower", aspect="auto",
                       cmap="viridis")
        ax.set_xticks(range(len(mults)))
        ax.set_xticklabels([f"x{m}" for m in mults])
        ax.set_yticks(range(len(blocks)))
        ax.set_yticklabels([f"{b}" for b in blocks])
        ax.set_xlabel("grid = SMs * m")
        if panel == 0:
            ax.set_ylabel("block size")
        ax.set_title(f"{platform.title()} GPU  N={int(n)}  (median of {int(sub['nruns'].iloc[0])} reps)")

        r, c = np.unravel_index(np.nanargmax(grid.values), grid.shape)
        ax.add_patch(plt.Rectangle((c - 0.5, r - 0.5), 1, 1,
                                    fill=False, edgecolor="red", linewidth=2))
        fig.colorbar(im, ax=ax, shrink=0.8,
                     label="GB/s" if panel == len(ns) - 1 else "")


def main():
    # ---- CPU figure (one column per platform) ----
    fig_cpu, axes_cpu = plt.subplots(
        1, len(PLATFORMS),
        figsize=(6.5 * len(PLATFORMS), 4.2),
        squeeze=False,
    )
    for i, plat in enumerate(PLATFORMS):
        df = load_csv(EXP_DIR, "numa_cpu.csv", platform=plat)
        plot_cpu(axes_cpu[0, i], df, plat)
    fig_cpu.tight_layout()
    cpu_pdf = fig_output_path(EXP_DIR, "numa_cpu")
    fig_cpu.savefig(cpu_pdf)
    print(f"wrote {cpu_pdf}")

    # ---- GPU figure (one row per platform; columns = N values) ----
    df_gpu = load_csv(EXP_DIR, "numa_gpu.csv")   # all platforms concatenated
    if df_gpu.empty:
        print("no GPU CSVs found; skipping GPU figure", file=sys.stderr)
        return
    ns_ref = sorted(df_gpu["N"].unique())
    fig_gpu, axes_gpu = plt.subplots(
        len(PLATFORMS), len(ns_ref),
        figsize=(4.6 * len(ns_ref), 3.8 * len(PLATFORMS)),
        squeeze=False,
    )
    for i, plat in enumerate(PLATFORMS):
        sub = df_gpu[df_gpu["platform"] == plat]
        plot_gpu_heatmaps(fig_gpu, axes_gpu[i], sub, plat)
    fig_gpu.tight_layout()
    gpu_pdf = fig_output_path(EXP_DIR, "numa_gpu")
    fig_gpu.savefig(gpu_pdf)
    print(f"wrote {gpu_pdf}")


if __name__ == "__main__":
    main()
