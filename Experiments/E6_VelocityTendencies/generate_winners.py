#!/usr/bin/env python3
"""
generate_winners.py

Read the per-loopnest sweep results and emit a layout cross-product
JSON whose per-group V-id sets come from the empirical winners (with
a majority-vote promotion rule), not a hardcoded list.

Pipeline:
  1. For each (loopnest, platform) pair, pick the lowest-time non-blocked
     V-id from the GPU sweep CSV at the chosen ``--nlev`` slice.
  2. Map each loopnest's winners into the access groups it touches via
     ``access_analysis/canonical_array_groups.json``. The result is
     ``winner_sets[group_id] = set of V-ids that won at least once``.
  3. Promotion rule: find the largest set ``S`` such that more than
     ``--threshold`` (default 50%) of the *non-empty* per-group winner
     sets are subsets of ``S``. If such an ``S`` exists, every group whose
     winner set is a subset of ``S`` (including empty groups, which
     represent missing coverage) is promoted to ``S``.
  4. Emit the full cross-product over the promoted per-group sets in
     the same JSON shape as ``layout_crossproduct_v123.json`` so that
     ``E7/tools/run_layout_configs.py`` can consume it unchanged.

Usage:
    python3 generate_winners.py
    python3 generate_winners.py --nlev 90 --backend gpu
    python3 generate_winners.py --threshold 0.5 --no-promotion
    python3 generate_winners.py --out full_velocity_tendencies/layout_crossproduct_winners.json
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
DEFAULT_OUT_JSON = EXP_DIR / "full_velocity_tendencies" / "layout_crossproduct_winners.json"


# ──────────────────────────────────────────────────────────────────────
#  Loaders
# ──────────────────────────────────────────────────────────────────────

def load_groups():
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
    """Return ``{loopnest_id: set(array_names_in_chosen_nest)}`` from
    layout_candidates.json. Falls back to empty if the file is missing."""
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


def median_unblocked(csv_path, ln_id, backend, nlev, cell_dist):
    """Return ``{layout_id: median_time_ms}`` for non-blocked V-ids
    matching the requested ``nlev`` and ``cell_dist`` slice."""
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
    """Return list of ``{loopnest, platform, best_V_id, time_ms}`` rows."""
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


# ──────────────────────────────────────────────────────────────────────
#  Per-group winners + promotion
# ──────────────────────────────────────────────────────────────────────

def winner_sets_per_group(winners, group_arrays, ln_arr):
    """Build ``{group_id: set(V-ids)}`` from the per-loopnest winners.
    A V-id wins for a group if it was the best for at least one
    (loopnest, platform) whose loopnest touches an array in the group."""
    out = {gid: set() for gid in group_arrays}
    for w in winners:
        ln = w["loopnest"]
        ln_arrs = ln_arr.get(ln, set())
        for gid, group_arrs in group_arrays.items():
            if any(a in group_arrs for a in ln_arrs):
                out[gid].add(w["best_V_id"])
    return out


def _frozen(s):
    return frozenset(s)


def promote(winner_sets, threshold):
    """Apply the >threshold promotion rule.

    Find the largest non-empty winner set ``S`` such that more than
    ``threshold`` of the *non-empty* per-group winner sets are subsets
    of ``S``. If such an ``S`` exists, every group whose winner set is
    a subset of ``S`` (including empty groups) is promoted to ``S``.

    Returns ``(promoted_dict, fired_set_or_None)``.
    """
    nonempty = {gid: s for gid, s in winner_sets.items() if s}
    if not nonempty:
        return dict(winner_sets), None

    # Candidate sets are the distinct non-empty winner sets observed.
    candidates = sorted({_frozen(s) for s in nonempty.values()},
                        key=lambda s: (-len(s), tuple(sorted(s))))

    n_nonempty = len(nonempty)
    fired = None
    fired_count = -1
    for cand in candidates:
        n_subset = sum(1 for s in nonempty.values() if _frozen(s) <= cand)
        if n_subset / n_nonempty > threshold:
            # Pick the largest set; tiebreak on most matches.
            if (fired is None
                    or len(cand) > len(fired)
                    or (len(cand) == len(fired) and n_subset > fired_count)):
                fired = cand
                fired_count = n_subset

    if fired is None:
        return dict(winner_sets), None

    promoted = {}
    for gid, s in winner_sets.items():
        if not s or _frozen(s) <= fired:
            promoted[gid] = set(fired)
        else:
            # Group's winners are NOT a subset of the promoted set --
            # keep them as-is so we don't drop a confirmed winner.
            promoted[gid] = set(s)
    return promoted, set(fired)


# ──────────────────────────────────────────────────────────────────────
#  Cross product
# ──────────────────────────────────────────────────────────────────────

def project(lid, axis):
    try:
        v = project_layout_to_axis(lid, axis)
    except Exception:
        v = None
    return v if v is not None else "unknown"


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--nlev", type=int, default=256,
                    help="nlev slice consulted for per-loopnest winners "
                         "(default: 256)")
    ap.add_argument("--cell-dist", default="exact",
                    help="cell_dist filter for the winner pick "
                         "('exact', 'uniform', or '' for none)")
    ap.add_argument("--backend", default="gpu", choices=["gpu", "cpu"],
                    help="benchmark backend (default: gpu)")
    ap.add_argument("--threshold", type=float, default=0.5,
                    help="promotion threshold; promote to a set S iff "
                         ">threshold of non-empty per-group winner sets "
                         "are subsets of S (default: 0.5)")
    ap.add_argument("--no-promotion", action="store_true",
                    help="skip the promotion rule -- use raw per-group "
                         "winners (empty groups stay empty)")
    ap.add_argument("--uncovered-fallback", default="V1,V2,V6",
                    help="V-ids to use for groups whose winner set is "
                         "still empty after promotion (default: V1,V2,V6); "
                         "set to '' to leave them empty")
    ap.add_argument("--out", type=Path, default=DEFAULT_OUT_JSON,
                    help=f"output JSON path (default: "
                         f"{DEFAULT_OUT_JSON.relative_to(EXP_DIR)})")
    ap.add_argument("--no-csv", action="store_true",
                    help="skip the side-by-side CSV")
    args = ap.parse_args()

    cell_dist = args.cell_dist or None
    fallback = [v.strip() for v in args.uncovered_fallback.split(",") if v.strip()]

    groups = load_groups()
    group_arrays = {g["group_id"]: set(g["arrays"]) for g in groups}
    ln_arr = loopnest_to_arrays()

    winners = best_v_per_loopnest_platform(args.nlev, cell_dist, args.backend)
    raw_sets = winner_sets_per_group(winners, group_arrays, ln_arr)

    if args.no_promotion:
        promoted = {gid: set(s) for gid, s in raw_sets.items()}
        fired = None
    else:
        promoted, fired = promote(raw_sets, args.threshold)

    # Empty-group fallback: groups still empty after promotion get the
    # CLI fallback (default V1,V2,V6) so the cross-product never has
    # an empty axis.
    for gid, s in promoted.items():
        if not s and fallback:
            promoted[gid] = set(fallback)

    print(f"[generate_winners] nlev={args.nlev} cell_dist={cell_dist} "
          f"backend={args.backend}")
    print(f"  measured winners: {len(winners)} (loopnest, platform) entries")
    print(f"  promotion threshold: {args.threshold} "
          f"({'fired -> ' + ','.join(sorted(fired)) if fired else 'did NOT fire'})")
    print(f"  per-group V-id sets:")
    for g in groups:
        gid = g["group_id"]
        raw = sorted(raw_sets[gid])
        prom = sorted(promoted[gid])
        suffix = "" if raw == prom else f"  (raw winners: {raw})"
        print(f"    {gid:<3} axis={g['axis']:<3} -> {prom}{suffix}")

    n_configs = 1
    for g in groups:
        n_configs *= len(promoted[g["group_id"]])
    print(f"  cross-product cells: {n_configs}")

    # Build the cross product. Schema matches layout_crossproduct_v123.json
    # so E7/tools/run_layout_configs.py consumes it unchanged.
    group_axes = [(g, sorted(promoted[g["group_id"]])) for g in groups]
    configs = []
    for combo in product(*[lst for _, lst in group_axes]):
        cfg = {"id": len(configs)}
        for (g, _), lid in zip(group_axes, combo):
            gid = g["group_id"]
            cfg[gid] = lid
            cfg[f"{gid}_{g['axis']}"] = project(lid, g["axis"])
        configs.append(cfg)

    axis_signatures = {tuple(cfg[f"{g['group_id']}_{g['axis']}"] for g in groups)
                       for cfg in configs}
    print(f"  unique configs after IC/IN axis projection: {len(axis_signatures)}")

    out = {
        "meta": {
            "source": "generate_winners.py",
            "nlev": args.nlev,
            "cell_dist": cell_dist,
            "backend": args.backend,
            "threshold": args.threshold,
            "no_promotion": args.no_promotion,
            "uncovered_fallback": fallback,
            "promoted_set": sorted(fired) if fired else None,
            "raw_winner_sets_per_group": {gid: sorted(s) for gid, s in raw_sets.items()},
            "promoted_winner_sets_per_group": {gid: sorted(s) for gid, s in promoted.items()},
            "n_groups": len(groups),
            "n_configs": n_configs,
            "n_unique_after_axis_projection": len(axis_signatures),
            "group_axis": {g["group_id"]: g["axis"] for g in groups},
        },
        "array_groups": groups,
        "winners": winners,
        "crossproduct": configs,
    }

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(out, indent=2))
    rel = args.out.relative_to(EXP_DIR) if args.out.is_relative_to(EXP_DIR) else args.out
    print(f"  wrote {rel}")

    if not args.no_csv:
        csv_path = args.out.with_suffix(".csv")
        with open(csv_path, "w", newline="") as f:
            w = csv.writer(f)
            header = ["id"]
            for g in groups:
                header += [g["group_id"], f"{g['group_id']}_{g['axis']}"]
            w.writerow(header)
            for cfg in configs:
                row = [cfg["id"]]
                for g in groups:
                    row += [cfg[g["group_id"]], cfg[f"{g['group_id']}_{g['axis']}"]]
                w.writerow(row)
        rel = csv_path.relative_to(EXP_DIR) if csv_path.is_relative_to(EXP_DIR) else csv_path
        print(f"  wrote {rel}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
