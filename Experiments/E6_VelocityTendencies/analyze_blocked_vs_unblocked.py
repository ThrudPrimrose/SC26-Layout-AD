#!/usr/bin/env python3
"""
analyze_blocked_vs_unblocked.py

Answer the keep-or-drop question: for every loopnest x platform x backend,
is the best BLOCKED layout (B{n} / t{TX}x{TY} storage) meaningfully faster
than the best UNBLOCKED variant (V1..V4)?

Inputs  : every loopnest_{1..6}/results/{beverin,daint}/*_{cpu,gpu}.csv
Output  : stdout table + optional machine-readable summary (JSON).

Per-cell columns:
    best_V_id        best unblocked layout id (V1..V4)
    best_V_ms        its median time_ms
    best_B_id        best blocked layout id (B{n} or t{TX}x{TY})
    best_B_ms        its median time_ms
    speedup          best_V_ms / best_B_ms      (>1  ->  blocked is faster)
    verdict          "keep blocked" / "drop blocked"

Default threshold: keep blocked iff speedup >= 1.05 (>=5% faster) on at
least one platform/backend. Tighten with --threshold to be stricter.
"""

import argparse
import csv
import json
import statistics
import sys
from collections import defaultdict
from pathlib import Path

EXP_DIR = Path(__file__).resolve().parent
LOOPNEST_IDS = [1, 2, 3, 4, 5, 6]
PLATFORMS = ("beverin", "daint")
ALL_BACKENDS = ("cpu", "gpu")


def _csv_for(ln_id, platform, backend):
    d = EXP_DIR / f"loopnest_{ln_id}" / "results" / platform
    if not d.is_dir():
        return None
    for p in sorted(d.glob(f"*_{backend}.csv")):
        name = p.name
        if name.startswith("metrics") or name.endswith("_old.csv"):
            continue
        return p
    return None


def _layout_id_and_blocked(row, schema, backend):
    """Return (layout_id, is_blocked). Mirrors build_layout_crossproduct.py
    normalization; duplicated here so this script is self-contained."""
    if schema == "l1_cpu":
        V = int(row["variant"]); B = int(row["blocking"])
        if V > 0 and B == 0:
            return f"V{V}", False
        if V == 0 and B > 0:
            return f"B{B}", True
        return f"V{V}_B{B}", (B > 0)
    if schema == "l1_gpu":
        V = int(row["variant"])
        # V=0 on L1 GPU corresponds to tiled-storage (config_label: t{TX}x{TY}).
        if V == 0:
            prefix = row.get("config_label", "").split("_", 1)[0]
            return prefix or "B?", True
        return f"V{V}", False
    # new-schema (L2-6)
    if backend == "cpu":
        lay = str(row["layout"])
        if lay.isdigit():
            return f"B{lay}", True
        return lay, lay.startswith("B")
    # new-schema GPU
    cfg = row["config_label"]
    if cfg.startswith("B"):
        return cfg.split("_", 1)[0], True
    V = int(row["V"])
    return f"V{V}", False


def _detect_schema(header, backend):
    if "variant" in header:
        return "l1_cpu" if backend == "cpu" else "l1_gpu"
    if "V" in header:
        return "new_cpu" if backend == "cpu" else "new_gpu"
    raise RuntimeError(f"unknown CSV schema: {list(header)}")


def load_medians_split(csv_path, backend, nlev, cell_dist):
    """Return (unblocked_medians, blocked_medians) as two dicts
    {layout_id: median_time_ms}."""
    rows = list(csv.DictReader(open(csv_path)))
    if not rows:
        return {}, {}
    schema = _detect_schema(rows[0].keys(), backend)
    nlev_str = str(nlev)
    unblk = defaultdict(list)
    blk = defaultdict(list)
    for r in rows:
        if r.get("nlev") != nlev_str:
            continue
        if cell_dist and r.get("cell_dist") != cell_dist:
            continue
        try:
            lid, is_blocked = _layout_id_and_blocked(r, schema, backend)
            t = float(r["time_ms"])
        except (KeyError, ValueError):
            continue
        (blk if is_blocked else unblk)[lid].append(t)
    return (
        {k: statistics.median(v) for k, v in unblk.items() if v},
        {k: statistics.median(v) for k, v in blk.items() if v},
    )


def analyze_one(ln_id, platform, backend, nlev, cell_dist):
    csv_path = _csv_for(ln_id, platform, backend)
    if csv_path is None:
        return None
    unblk, blk = load_medians_split(csv_path, backend, nlev, cell_dist)
    if not unblk and not blk:
        return None
    row = {
        "loopnest": ln_id,
        "platform": platform,
        "backend": backend,
        "csv": str(csv_path.relative_to(EXP_DIR)),
    }
    if unblk:
        bv = min(unblk, key=unblk.get)
        row["best_V_id"] = bv
        row["best_V_ms"] = unblk[bv]
        row["n_unblocked_measured"] = len(unblk)
    if blk:
        bb = min(blk, key=blk.get)
        row["best_B_id"] = bb
        row["best_B_ms"] = blk[bb]
        row["n_blocked_measured"] = len(blk)
    if unblk and blk:
        row["speedup"] = row["best_V_ms"] / row["best_B_ms"]
    return row


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--nlev", default=96, type=int)
    ap.add_argument("--cell-dist", default="exact")
    ap.add_argument("--threshold", default=1.25, type=float,
                    help="blocked kept iff max-speedup >= threshold "
                         "(default 1.25 = 25%% faster than unblocked). "
                         "Anything below this ratio is not worth the "
                         "layout-transformation overhead and is dropped "
                         "from the sweep candidate set.")
    ap.add_argument("--json", default=None, type=Path,
                    help="optional JSON summary path")
    ap.add_argument("--backend", default="all",
                    choices=["gpu", "cpu", "all"],
                    help="restrict the blocked-vs-unblocked verdict to "
                         "GPU-only or CPU-only CSVs. A loopnest is marked "
                         "KEEP blocked iff its max speedup across the "
                         "selected backends meets --threshold.")
    args = ap.parse_args()
    cell_dist = args.cell_dist or None
    backends = ALL_BACKENDS if args.backend == "all" else (args.backend,)

    header = ("LN Platform Backend | "
              "best-V    V-ms     | best-B   B-ms     | speedup  verdict")
    print(f"# blocked-vs-unblocked, nlev={args.nlev}, cell_dist={cell_dist}, "
          f"backend={args.backend}")
    print(f"# keep-threshold: speedup >= {args.threshold:.2f}")
    print()
    print(header)
    print("-" * len(header))

    all_rows = []
    per_loopnest_best_speedup = defaultdict(lambda: 1.0)
    for ln in LOOPNEST_IDS:
        for plat in PLATFORMS:
            for be in backends:
                r = analyze_one(ln, plat, be, args.nlev, cell_dist)
                if r is None:
                    continue
                all_rows.append(r)
                if "speedup" in r:
                    if r["speedup"] > per_loopnest_best_speedup[ln]:
                        per_loopnest_best_speedup[ln] = r["speedup"]
                v_id = r.get("best_V_id", "-")
                v_ms = r.get("best_V_ms")
                b_id = r.get("best_B_id", "-")
                b_ms = r.get("best_B_ms")
                sp = r.get("speedup")
                verdict = ""
                if sp is not None:
                    verdict = "KEEP  blocked" if sp >= args.threshold else "drop  blocked"
                sp_str = f"{sp:6.2f}x" if sp is not None else "  - "
                v_ms_s = f"{v_ms:8.4f}" if v_ms is not None else "     -  "
                b_ms_s = f"{b_ms:8.4f}" if b_ms is not None else "     -  "
                print(f"{ln:>2} {plat:>7} {be:>3}    | "
                      f"{v_id:<5} {v_ms_s} | "
                      f"{b_id:<6} {b_ms_s} | "
                      f"{sp_str}   {verdict}")

    print()
    print("# per-loopnest max speedup (across all platforms/backends):")
    verdict_per_ln = {}
    for ln in LOOPNEST_IDS:
        sp = per_loopnest_best_speedup[ln]
        keep = sp >= args.threshold
        verdict_per_ln[ln] = {"max_speedup": sp, "keep_blocked": keep}
        print(f"  loopnest_{ln}: {sp:.2f}x  -> "
              f"{'KEEP blocked' if keep else 'DROP blocked'}")

    if args.json:
        with open(args.json, "w") as f:
            json.dump({
                "meta": {"nlev": args.nlev, "cell_dist": cell_dist,
                         "threshold": args.threshold},
                "rows": all_rows,
                "verdict_per_loopnest": verdict_per_ln,
            }, f, indent=2)
        print(f"\n# wrote {args.json}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
