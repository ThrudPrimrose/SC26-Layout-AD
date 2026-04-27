#!/usr/bin/env python3
"""
generate_v123_candidates.py

Emit a layout-cross-product JSON where every canonical array group
(cv / ch / f / s / n / lm) is independently assigned a V-id.

Default per-group set is **V1, V2, V6** — these three together span
both axes minimally and contain every V-id that was empirically a
winner in the 6-loopnest sweep (V1, V2, V6). They are: V1=h_first/SoA,
V2=h_first/AoS, V6=v_first/AoS. (V3=v_first/SoA is the alternate
v_first/IN corner and can be added with --covered V1,V2,V3,V6.)

Per-group layout set is chosen by *coverage*:

  - groups with at least one empirical winner from the loopnest sweep
    use the "covered" set (default V1, V2, V6),
  - groups with NO empirical winner (e.g. `n` because LOKI strips
    connectivity arrays from per-nest lists, or `lm` because it is a
    fresh group introduced for the levmask/levelmask layout switch)
    use the "uncovered" set (default V1, V2, V6).

Total configs = (per_group_set_size)**n_groups, and each config also
records the projection of its V-id onto the group's storage axis
(IC for cv/ch/f/s/lm, IN for n) so downstream consumers can dedupe at
the IC/IN level when desired.

When the daint repo holds a deep copy of the beverin CSVs (under
loopnest_{1..6}/results/beverin/), this script also pulls the per-(loopnest,
platform) median timings, picks each loopnest's best V-id, and annotates
each candidate config with empirical evidence per group:

  - "<group>_evidence": list of {loopnest, platform, time_ms} rows where
    the loopnest is one that touches this group AND the chosen V-id
    matched at least one platform's measured winner.
  - "n_groups_with_evidence": int — quick summary.

Output path defaults to
  full_velocity_tendencies/layout_crossproduct_v123.json

Run:
  /usr/bin/python3.11 generate_v123_candidates.py
  /usr/bin/python3.11 generate_v123_candidates.py --covered V1,V2,V3 --uncovered V1,V2,V6
  /usr/bin/python3.11 generate_v123_candidates.py --no-evidence
"""

import argparse
import csv
import json
import statistics
import sys
from collections import defaultdict
from itertools import product
from pathlib import Path

from _analysis_util import (
    EXP_DIR,
    LOOPNEST_IDS,
    PLATFORMS,
    csv_for,
    detect_schema,
    extract_layout_id,
    is_blocked_layout_id,
    project_layout_to_axis,
)

CANONICAL_GROUPS_JSON = EXP_DIR / "access_analysis" / "canonical_array_groups.json"
LAYOUT_CANDIDATES_JSON = EXP_DIR / "access_analysis" / "layout_candidates.json"
DEFAULT_OUT_JSON = EXP_DIR / "full_velocity_tendencies" / "layout_crossproduct_v123.json"


# ──────────────────────────────────────────────────────────────────────
#  group / loopnest taxonomy
# ──────────────────────────────────────────────────────────────────────

def load_groups():
    """Read canonical_array_groups.json and return a list of group records
    in fixed order: cv, ch, f, s, n."""
    data = json.loads(CANONICAL_GROUPS_JSON.read_text())
    out = []
    for gid in data["group_ids"]:
        out.append({
            "group_id": gid,
            "label": data["group_labels"][gid],
            "axis": data["group_axis"][gid],
            "n_arrays": len(data["arrays"][gid]),
            "arrays": data["arrays"][gid],
        })
    return out


def loopnest_to_arrays():
    """Return {loopnest_id: set(array_names)} from layout_candidates.json,
    using the chosen_nid mapping. Falls back to empty if file is missing."""
    if not LAYOUT_CANDIDATES_JSON.is_file():
        return {ln: set() for ln in LOOPNEST_IDS}
    data = json.loads(LAYOUT_CANDIDATES_JSON.read_text())
    nests = {n["nid"]: n for n in data.get("all_nests", [])}
    pinned = {int(k.split("_", 1)[1]): v
              for k, v in data.get("pinned", {}).items()
              if k.startswith("loopnest_")}
    out = {}
    for ln in LOOPNEST_IDS:
        nid = pinned.get(ln, ln)
        n = nests.get(nid)
        out[ln] = set(n.get("arrays", [])) if n else set()
    return out


def groups_touched_by_loopnest(group_arrays, ln_arrays):
    """Return the set of group_ids touched by this loopnest's array list."""
    touched = set()
    for gid, arrs in group_arrays.items():
        if any(a in arrs for a in ln_arrays):
            touched.add(gid)
    return touched


# ──────────────────────────────────────────────────────────────────────
#  empirical winners per (loopnest, platform)
# ──────────────────────────────────────────────────────────────────────

def median_unblocked(csv_path, ln_id, backend, nlev, cell_dist):
    """Return {layout_id: median_time_ms} restricted to non-blocked V-ids."""
    if csv_path is None or not csv_path.exists():
        return {}
    rows = list(csv.DictReader(open(csv_path)))
    if not rows:
        return {}
    schema = detect_schema(rows[0].keys(), ln_id, backend)
    nlev_str = str(nlev)
    bucket = defaultdict(list)
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
        if is_blocked_layout_id(lid):
            continue
        bucket[lid].append(t)
    return {k: statistics.median(v) for k, v in bucket.items() if v}


def best_v_per_loopnest_platform(nlev, cell_dist, backend):
    """Return list of {loopnest, platform, best_V_id, time_ms} entries."""
    out = []
    for ln in LOOPNEST_IDS:
        for plat in PLATFORMS:
            csv_path = csv_for(ln, plat, backend)
            d = median_unblocked(csv_path, ln, backend, nlev, cell_dist)
            if not d:
                continue
            best = min(d, key=d.get)
            out.append({
                "loopnest": ln,
                "platform": plat,
                "best_V_id": best,
                "time_ms": d[best],
            })
    return out


def evidence_index(winners, group_arrays, ln_arr):
    """Build {(group_id, V-id): [winner-row, ...]} index of which V-ids
    are empirically validated for each group."""
    idx = defaultdict(list)
    for w in winners:
        ln = w["loopnest"]
        arrs = ln_arr.get(ln, set())
        touched = groups_touched_by_loopnest(group_arrays, arrs)
        for gid in touched:
            idx[(gid, w["best_V_id"])].append(w)
    return idx


# ──────────────────────────────────────────────────────────────────────
#  cross-product builder
# ──────────────────────────────────────────────────────────────────────

def project(lid, axis):
    """Wrap project_layout_to_axis with a graceful fallback for unknown
    V-ids — keeps the JSON useful if the user widens the layout set."""
    try:
        v = project_layout_to_axis(lid, axis)
    except Exception:
        v = None
    return v if v is not None else "unknown"


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--covered", default="V1,V2,V6",
                    help="V-ids to sweep for groups WITH empirical "
                         "evidence (default: V1,V2,V6 — matches the actual "
                         "empirical winners; V1/V2 cover h_first, V6 covers "
                         "v_first/AoS which is what L1 measured as best)")
    ap.add_argument("--uncovered", default="V1,V2,V6",
                    help="V-ids to sweep for groups WITHOUT empirical "
                         "evidence (default: V1,V2,V6)")
    ap.add_argument("--nlev", default=128, type=int,
                    help="nlev slice to consult for empirical evidence "
                         "(default: 128, matches §IV-D / Table IV)")
    ap.add_argument("--cell-dist", default="exact",
                    help="cell_dist filter for evidence ('exact', 'uniform', "
                         "or '' for none)")
    ap.add_argument("--backend", default="gpu",
                    choices=["gpu", "cpu"],
                    help="backend whose CSVs are consulted (default: gpu)")
    ap.add_argument("--no-evidence", action="store_true",
                    help="skip empirical-evidence annotations; pure "
                         "cross-product only (uses the --covered set "
                         "uniformly because no coverage info exists)")
    ap.add_argument("--out", default=str(DEFAULT_OUT_JSON), type=Path,
                    help=f"output JSON path (default: "
                         f"{DEFAULT_OUT_JSON.relative_to(EXP_DIR)})")
    ap.add_argument("--no-csv", action="store_true",
                    help="skip the side-by-side CSV")
    args = ap.parse_args()

    covered = [s.strip() for s in args.covered.split(",") if s.strip()]
    uncovered = [s.strip() for s in args.uncovered.split(",") if s.strip()]
    if not covered or not uncovered:
        print("error: --covered/--uncovered must be non-empty",
              file=sys.stderr)
        return 1

    cell_dist = args.cell_dist or None

    groups = load_groups()
    n_groups = len(groups)

    # Empirical evidence — also used to decide per-group layout set.
    winners = []
    coverage = defaultdict(set)        # group_id -> set of loopnests
    evidence = defaultdict(list)       # (group_id, V-id) -> [winner rows]
    if not args.no_evidence:
        group_arrays = {g["group_id"]: set(g["arrays"]) for g in groups}
        ln_arr = loopnest_to_arrays()
        winners = best_v_per_loopnest_platform(args.nlev, cell_dist, args.backend)
        evidence = evidence_index(winners, group_arrays, ln_arr)
        for w in winners:
            ln = w["loopnest"]
            arrs = ln_arr.get(ln, set())
            for gid in groups_touched_by_loopnest(group_arrays, arrs):
                coverage[gid].add(ln)

    # Per-group layout set: covered / uncovered split.
    per_group_layouts = {}
    for g in groups:
        gid = g["group_id"]
        per_group_layouts[gid] = covered if coverage.get(gid) else uncovered

    n_configs = 1
    for g in groups:
        n_configs *= len(per_group_layouts[g["group_id"]])

    print(f"[generate_v123_candidates] groups={n_groups} -> {n_configs} configs")
    print(f"  covered set:   {covered}")
    print(f"  uncovered set: {uncovered}")
    if not args.no_evidence:
        print(f"  empirical winners measured: {len(winners)} "
              f"(nlev={args.nlev}, cell_dist={cell_dist}, "
              f"backend={args.backend})")
    for g in groups:
        gid = g["group_id"]
        layouts = per_group_layouts[gid]
        kind = "COVERED" if coverage.get(gid) else "uncovered"
        cv_set = sorted({w["best_V_id"]
                        for (g2, _), ws in evidence.items()
                        if g2 == gid for w in ws})
        cov_lns = sorted(coverage[gid])
        print(f"  {gid:<3} axis={g['axis']:<3} {kind:<9} "
              f"layouts={layouts}  ({g['n_arrays']} arrays, "
              f"touching_LN={cov_lns}, V-winners={cv_set})")

    # Cross product: each group independently picks from its own layout set.
    group_axes = [(g, per_group_layouts[g["group_id"]]) for g in groups]
    configs = []
    for combo in product(*[lst for _, lst in group_axes]):
        cfg = {"id": len(configs)}
        n_with_evidence = 0
        for (g, _), lid in zip(group_axes, combo):
            gid = g["group_id"]
            cfg[gid] = lid
            cfg[f"{gid}_{g['axis']}"] = project(lid, g["axis"])
            if not args.no_evidence:
                ev = evidence.get((gid, lid), [])
                cfg[f"{gid}_evidence"] = [
                    {"loopnest": e["loopnest"],
                     "platform": e["platform"],
                     "time_ms": e["time_ms"]}
                    for e in ev
                ]
                if ev:
                    n_with_evidence += 1
        if not args.no_evidence:
            cfg["n_groups_with_evidence"] = n_with_evidence
        configs.append(cfg)

    # Per-axis dedup count.
    axis_signatures = {tuple(cfg[f"{g['group_id']}_{g['axis']}"] for g in groups)
                       for cfg in configs}
    print(f"  unique configs after axis projection: {len(axis_signatures)}")

    out = {
        "meta": {
            "source": "generate_v123_candidates.py",
            "covered_layouts": covered,
            "uncovered_layouts": uncovered,
            "layouts_per_group": {g["group_id"]: per_group_layouts[g["group_id"]]
                                   for g in groups},
            "n_groups": n_groups,
            "n_configs": n_configs,
            "n_unique_after_axis_projection": len(axis_signatures),
            "group_axis": {g["group_id"]: g["axis"] for g in groups},
            "evidence": (None if args.no_evidence else {
                "nlev": args.nlev,
                "cell_dist": cell_dist,
                "backend": args.backend,
                "n_winners": len(winners),
                "coverage_per_group": {gid: sorted(lns)
                                       for gid, lns in coverage.items()},
            }),
        },
        "array_groups": groups,
        "winners": winners,
        "crossproduct": configs,
    }

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(out, indent=2))
    print(f"  wrote {args.out.relative_to(EXP_DIR) if args.out.is_relative_to(EXP_DIR) else args.out}")

    if not args.no_csv:
        csv_path = args.out.with_suffix(".csv")
        with open(csv_path, "w", newline="") as f:
            w = csv.writer(f)
            header = ["id"]
            for g in groups:
                header += [g["group_id"], f"{g['group_id']}_{g['axis']}"]
            if not args.no_evidence:
                header += ["n_groups_with_evidence"]
            w.writerow(header)
            for cfg in configs:
                row = [cfg["id"]]
                for g in groups:
                    row += [cfg[g["group_id"]],
                            cfg[f"{g['group_id']}_{g['axis']}"]]
                if not args.no_evidence:
                    row += [cfg["n_groups_with_evidence"]]
                w.writerow(row)
        print(f"  wrote {csv_path.relative_to(EXP_DIR) if csv_path.is_relative_to(EXP_DIR) else csv_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
