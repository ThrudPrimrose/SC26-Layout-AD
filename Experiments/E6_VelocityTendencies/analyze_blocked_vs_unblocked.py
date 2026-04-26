#!/usr/bin/env python3
"""
analyze_blocked_vs_unblocked.py

For every loopnest × platform × backend, compare the best BLOCKED layout
(B{n} / t{TX}x{TY} storage) against the best UNBLOCKED variant
(V1..V4) and decide whether keeping blocked in the sweep is worth it.

A loopnest is marked KEEP blocked iff its max speedup across the
selected backends meets --threshold (default 1.25×). The output JSON
is consumed by `build_layout_crossproduct.py --blocked-verdict`.
"""

import argparse
import csv
import json
import statistics
import sys
from collections import defaultdict
from pathlib import Path

from _analysis_util import (
    ALL_BACKENDS,
    EXP_DIR,
    LOOPNEST_IDS,
    PLATFORMS,
    csv_for,
    detect_schema,
    extract_layout_id,
    is_blocked_layout_id,
)


def load_medians_split(csv_path, loopnest_id, backend, nlev, cell_dist):
    """Return (unblocked, blocked) dicts: {layout_id: median_time_ms}."""
    rows = list(csv.DictReader(open(csv_path)))
    if not rows:
        return {}, {}
    schema = detect_schema(rows[0].keys(), loopnest_id, backend)
    nlev_str = str(nlev)
    unblk, blk = defaultdict(list), defaultdict(list)
    for r in rows:
        if r.get("nlev") != nlev_str:
            continue
        if cell_dist and r.get("cell_dist") != cell_dist:
            continue
        try:
            lid = extract_layout_id(r, schema, backend)
            t = float(r["time_ms"])
        except (KeyError, ValueError, TypeError):
            continue
        (blk if is_blocked_layout_id(lid) else unblk)[lid].append(t)
    return (
        {k: statistics.median(v) for k, v in unblk.items() if v},
        {k: statistics.median(v) for k, v in blk.items() if v},
    )


def analyze_one(ln_id, platform, backend, nlev, cell_dist):
    csv_path = csv_for(ln_id, platform, backend)
    if csv_path is None:
        return None
    unblk, blk = load_medians_split(csv_path, ln_id, backend, nlev, cell_dist)
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
        row.update(best_V_id=bv, best_V_ms=unblk[bv],
                   n_unblocked_measured=len(unblk))
    if blk:
        bb = min(blk, key=blk.get)
        row.update(best_B_id=bb, best_B_ms=blk[bb],
                   n_blocked_measured=len(blk))
    if unblk and blk:
        row["speedup"] = row["best_V_ms"] / row["best_B_ms"]
    return row


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--nlev", default=96, type=int)
    ap.add_argument("--cell-dist", default="exact")
    ap.add_argument("--threshold", default=1.25, type=float,
                    help="blocked kept iff max-speedup >= threshold "
                         "(default 1.25×)")
    ap.add_argument("--backend", default="all",
                    choices=["gpu", "cpu", "all"],
                    help="restrict verdict to GPU-only / CPU-only CSVs")
    ap.add_argument("--json", default=None, type=Path,
                    help="optional JSON summary path")
    args = ap.parse_args()

    cell_dist = args.cell_dist or None
    backends = ALL_BACKENDS if args.backend == "all" else (args.backend,)

    header = ("LN Platform Backend | best-V    V-ms     "
              "| best-B   B-ms     | speedup  verdict")
    print(f"# blocked-vs-unblocked, nlev={args.nlev}, "
          f"cell_dist={cell_dist}, backend={args.backend}")
    print(f"# keep-threshold: speedup >= {args.threshold:.2f}")
    print()
    print(header)
    print("-" * len(header))

    all_rows = []
    per_ln_best = defaultdict(lambda: 1.0)
    for ln in LOOPNEST_IDS:
        for plat in PLATFORMS:
            for be in backends:
                r = analyze_one(ln, plat, be, args.nlev, cell_dist)
                if r is None:
                    continue
                all_rows.append(r)
                sp = r.get("speedup")
                if sp is not None and sp > per_ln_best[ln]:
                    per_ln_best[ln] = sp
                v_id = r.get("best_V_id", "-")
                b_id = r.get("best_B_id", "-")
                v_ms_s = f"{r['best_V_ms']:8.4f}" if "best_V_ms" in r else "     -  "
                b_ms_s = f"{r['best_B_ms']:8.4f}" if "best_B_ms" in r else "     -  "
                sp_str = f"{sp:6.2f}x" if sp is not None else "  - "
                verdict = ""
                if sp is not None:
                    verdict = "KEEP  blocked" if sp >= args.threshold else "drop  blocked"
                print(f"{ln:>2} {plat:>7} {be:>3}    | "
                      f"{v_id:<5} {v_ms_s} | "
                      f"{b_id:<6} {b_ms_s} | "
                      f"{sp_str}   {verdict}")

    print()
    print("# per-loopnest max speedup (across selected backends):")
    verdict_per_ln = {}
    for ln in LOOPNEST_IDS:
        sp = per_ln_best[ln]
        keep = sp >= args.threshold
        verdict_per_ln[ln] = {"max_speedup": sp, "keep_blocked": keep}
        print(f"  loopnest_{ln}: {sp:.2f}x  -> "
              f"{'KEEP blocked' if keep else 'DROP blocked'}")

    if args.json:
        args.json.write_text(json.dumps({
            "meta": {"nlev": args.nlev, "cell_dist": cell_dist,
                     "threshold": args.threshold, "backend": args.backend},
            "rows": all_rows,
            "verdict_per_loopnest": verdict_per_ln,
        }, indent=2))
        print(f"\n# wrote {args.json}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
