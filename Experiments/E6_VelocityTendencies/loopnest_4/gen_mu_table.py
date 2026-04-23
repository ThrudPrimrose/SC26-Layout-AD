#!/usr/bin/env python3
"""
gen_mu_table.py — Merged LaTeX table + bar plots of µ (new-block count)
for the ddt_vn_vert stencil.

Usage:
    python gen_mu_table.py --csv metrics90.csv
    python gen_mu_table.py --csv metrics90.csv --dists uniform exact --out mu_tables.tex
"""
import argparse, csv, sys

ap = argparse.ArgumentParser()
ap.add_argument("--csv", required=True, help="metrics CSV from cost_metrics")
ap.add_argument("--dists", nargs="+", default=["uniform", "exact"],
                help="Distributions to include (default: uniform exact)")
ap.add_argument("--schedule", default=None,
                help="Filter to a single schedule (omp_for / omp_collapse2). "
                     "Default: pick best (lowest mu) per config.")
ap.add_argument("--blocks", nargs="+", type=int, default=[32, 64, 128],
                help="Blocking factors to include")
ap.add_argument("--out", default=None, help="Output .tex file (default: stdout)")
ap.add_argument("--no-plot", action="store_true")
args = ap.parse_args()

CPU_TARGET = "cpu_scalar"   # block_bytes=64
GPU_TARGET = "gpu_scalar"   # block_bytes=128

# ── Load CSV ──
rows = []
with open(args.csv) as f:
    for r in csv.DictReader(f):
        rows.append(r)
if not rows:
    print(f"ERROR: {args.csv} is empty or missing header", file=sys.stderr); sys.exit(1)

# ── Index ──
data = {}
for r in rows:
    tgt = r["target"]
    if tgt not in (CPU_TARGET, GPU_TARGET): continue
    dist = r["cell_dist"]
    if dist not in args.dists: continue
    V, B, sch = int(r["variant"]), int(r["blocking"]), r["schedule"]
    mu = float(r["mu"])
    data[(V, B, dist, tgt, sch)] = {"mu": mu}

# Best-schedule: lowest µ
best = {}
for (V, B, dist, tgt, sch), m in data.items():
    if args.schedule and sch != args.schedule: continue
    k = (V, B, dist, tgt)
    if k not in best or m["mu"] < best[k]["mu"]:
        best[k] = {**m, "schedule": sch}

def get_mu(V, B, dist, tgt):
    k = (V, B, dist, tgt)
    return best[k]["mu"] if k in best else None

# ── Config rows: V1, V4, then blocked ──
config_rows = [
    {"label": "V1 (Horizontal-First)", "label_short": "V1", "V": 1, "B": 0, "is_blocked": False},
    {"label": "V4 (Vertical-First)",   "label_short": "V4", "V": 4, "B": 0, "is_blocked": False},
]
for B in sorted(args.blocks):
    config_rows.append({"label": f"B{B}", "label_short": f"B{B}",
                        "V": 0, "B": B, "is_blocked": True})

dists = args.dists
dist_labels = {
    "uniform": "Uniform",
    "exact":   "Exact (R02B05)",
    "normal_var1": r"Normal ($\sigma{=}1$)",
    "normal_var4": r"Normal ($\sigma{=}2$)",
    "sequential": "Sequential",
}

# ── Text table ──
print(f"\n{'='*90}\n  Merged µ table\n{'='*90}\n")
hdr = f"  {'Config':>22}"
for d in dists: hdr += f"  |  {dist_labels.get(d,d):^20}"
print(hdr)
hdr2 = f"  {'':>22}"
for d in dists: hdr2 += f"  |  {'µ_64B':>9} {'µ_128B':>9}"
print(hdr2)
sep = f"  {'-'*(24 + len(dists) * 24)}"
print(sep)

for i, cr in enumerate(config_rows):
    V, B = cr["V"], cr["B"]
    if cr["is_blocked"] and (i == 0 or not config_rows[i-1]["is_blocked"]):
        print(sep)
    line = f"  {cr['label']:>22}"
    for d in dists:
        mc = get_mu(V, B, d, CPU_TARGET)
        mg = get_mu(V, B, d, GPU_TARGET)
        line += f"  |  {(f'{mc:.2f}' if mc else '---'):>9} {(f'{mg:.2f}' if mg else '---'):>9}"
    print(line)

# ── Per-column minima ──
col_mins = {}
for d in dists:
    for tgt in [CPU_TARGET, GPU_TARGET]:
        vals = [get_mu(cr["V"], cr["B"], d, tgt) for cr in config_rows
                if get_mu(cr["V"], cr["B"], d, tgt) is not None]
        col_mins[(d, tgt)] = min(vals) if vals else None

def fmt_tex(v, vmin):
    if v is None: return "---"
    s = f"{v:.2f}"
    return rf"\textbf{{{s}}}" if vmin is not None and abs(v - vmin) < 1e-9 else s

# ── LaTeX table ──
nd = len(dists)
out = sys.stdout if not args.out else open(args.out, "w")
print("\n\n% ─── LaTeX table: merged µ ───", file=out)
col_spec = "@{}l" + " rr" * nd + "@{}"
print(rf"""\begin{{table}}[!htbp]
\centering\small
\renewcommand{{\arraystretch}}{{1.15}}
\begin{{tabular}}{{{col_spec}}}
\toprule""", file=out)

top = "Config"
for d in dists:
    dl = dist_labels.get(d, d)
    top += rf" & \multicolumn{{2}}{{c}}{{{dl}}}"
print(top + r" \\", file=out)

for i, d in enumerate(dists):
    c0, c1 = 2 + i*2, 3 + i*2
    print(rf"\cmidrule(lr){{{c0}-{c1}}}", end="", file=out)
print("", file=out)

sub = ""
for d in dists:
    sub += r" & $\mu_{\text{64\,B}}$ & $\mu_{\text{128\,B}}$"
print(sub + r" \\", file=out)
print(r"\midrule", file=out)

printed_sep = False
for cr in config_rows:
    V, B = cr["V"], cr["B"]
    if cr["is_blocked"] and not printed_sep:
        print(r"\midrule", file=out); printed_sep = True
    label = cr["label"]
    line = label
    for d in dists:
        line += f" & {fmt_tex(get_mu(V, B, d, CPU_TARGET), col_mins.get((d, CPU_TARGET)))}"
        line += f" & {fmt_tex(get_mu(V, B, d, GPU_TARGET), col_mins.get((d, GPU_TARGET)))}"
    print(line + r" \\", file=out)

dist_list = ", ".join(dist_labels.get(d, d).lower() for d in dists)
print(rf"""\bottomrule
\end{{tabular}}
\caption{{Average new-block count~$\mu$ for the \texttt{{z\_v\_grad\_w}} stencil
under {dist_list} connectivity patterns.
$\mu_{{\text{{64\,B}}}}$ and $\mu_{{\text{{128\,B}}}}$ correspond to
64-byte (CPU) and 128-byte (GPU) cache lines.
V1 and V4 are unblocked layout variants
(V1~=~horizontal-first, V4~=~vertical-first);
B$k$ denotes blocking factor~$k$.
Note that blocking \emph{{increases}}~$\mu$ in this stencil
because each block-tile touches indirect arrays at scattered positions,
creating more unique cache-line accesses per step.
Bold marks the column minimum.}}
\label{{tab:mu-merged}}
\end{{table}}""", file=out)

if args.out: out.close(); print(f"\nWritten to {args.out}")

# ── Plots ──
if not args.no_plot:
    try:
        import matplotlib; matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        from matplotlib.ticker import AutoMinorLocator, MaxNLocator, FormatStrFormatter
        import numpy as np

        DIST_COLORS = {"uniform": "#e67e22", "exact": "#2980b9",
                       "normal_var1": "#27ae60", "sequential": "#9b59b6"}

        for cache_label, tgt, cache_bytes in [
            ("64B (CPU)", CPU_TARGET, 64), ("128B (GPU)", GPU_TARGET, 128)]:
            labels = [cr["label_short"] for cr in config_rows]
            x = np.arange(len(labels))
            width = 0.8 / len(dists)

            fig, ax = plt.subplots(figsize=(3.6 * max(2, len(config_rows)/3), 2.8))
            ax.set_box_aspect(2.8 / 4.8)

            for di, d in enumerate(dists):
                vals = [get_mu(cr["V"], cr["B"], d, tgt) or 0 for cr in config_rows]
                offset = (di - (len(dists)-1)/2) * width
                ax.bar(x + offset, vals, width*0.9,
                       label=dist_labels.get(d, d),
                       color=DIST_COLORS.get(d, "#888"), edgecolor="black",
                       linewidth=0.5, alpha=0.75)
                for xi, v in zip(x, vals):
                    if v > 0:
                        ax.text(xi + offset, v + 0.05, f"{v:.1f}",
                                ha="center", va="bottom", fontsize=9)

            ax.set_xticks(x); ax.set_xticklabels(labels, fontsize=9)
            ax.set_ylabel(r"$\mu$ (new blocks / step)", fontsize=11)
            ax.set_title(rf"$\mu$ at {cache_bytes}-byte cache lines", fontsize=15)
            ax.yaxis.set_major_locator(MaxNLocator(nbins=5, min_n_ticks=5))
            ax.yaxis.set_minor_locator(AutoMinorLocator(2))
            ax.yaxis.set_major_formatter(FormatStrFormatter("%.1f"))
            ax.grid(axis="y", which="major", alpha=0.25)
            ax.grid(axis="y", which="minor", alpha=0.12, linestyle=":")
            ax.set_axisbelow(True)

            n_ub = sum(1 for cr in config_rows if not cr["is_blocked"])
            if n_ub < len(config_rows):
                ax.axvline(x=n_ub - 0.5, color="gray", ls="--", lw=1.5, alpha=0.6)
                yt = ax.get_ylim()[1] * 0.95
                ax.text((n_ub-1)/2, yt, "unblocked",
                        ha="center", va="top", fontsize=9, fontstyle="italic", alpha=0.6)
                ax.text(n_ub + (len(config_rows)-n_ub-1)/2, yt, "blocked",
                        ha="center", va="top", fontsize=9, fontstyle="italic", alpha=0.6)

            ax.legend(fontsize=10, loc="upper left")
            fig.tight_layout()
            tag = f"mu_{cache_bytes}B"
            fig.savefig(f"{tag}.png", dpi=200, bbox_inches="tight")
            fig.savefig(f"{tag}.pdf", dpi=200, bbox_inches="tight")
            plt.close(fig)
            print(f"  Plot saved: {tag}.png/pdf")
    except ImportError:
        print("\n  [WARN] matplotlib not available", file=sys.stderr)

print("\nDone.")