#!/usr/bin/env python3
"""
build_layout_crossproduct.py

Prepare the input list for the full velocity-tendencies sweep, following
section IV-A of the paper ("Searching the Transformation Space"):

  Step 3-4: per representative loop nest, pick the top-k (=2) winning
            layouts from the measured sweep (CSV under
            loopnest_{1..6}/results/*/).

  Step 5:   take the Cartesian product of those winners across the 5
            canonical array groups (cv / ch / f / s / n); groups no
            representative loop nest covers default to identity.

Inputs
------
  access_analysis/layout_candidates.json       chosen_nid + arrays per nest
  access_analysis/canonical_array_groups.json  cv / ch / f / s / n taxonomy
  loopnest_{1..6}/results/{beverin,daint}/*.csv benchmark sweep

Outputs
-------
  full_velocity_tendencies/layout_crossproduct.json   full record
  full_velocity_tendencies/layout_crossproduct.csv    one row per config
"""

import argparse
import csv
import json
import math
import sys
from collections import defaultdict
from itertools import product
from pathlib import Path

from _analysis_util import (
    ALL_BACKENDS,
    EXP_DIR,
    IDENTITY_LAYOUT_ID,
    LOOPNEST_IDS,
    PLATFORMS,
    csv_for,
    is_blocked_layout_id,
    load_medians,
    project_layout_to_axis,
)

ACCESS_JSON = EXP_DIR / "access_analysis" / "layout_candidates.json"
CANONICAL_GROUPS_JSON = EXP_DIR / "access_analysis" / "canonical_array_groups.json"
DEFAULT_OUT_DIR = EXP_DIR / "full_velocity_tendencies"


# ──────────────────────────────────────────────────────────────────────
#  Per-loopnest winner ranking
# ──────────────────────────────────────────────────────────────────────

def collect_winners(loopnest_id, nlev, cell_dist, k, drop_blocked=False,
                    backends=ALL_BACKENDS):
    """Rank layouts per loopnest by GEOMEAN of normalized medians across
    every measured {platform, backend} target in `backends`.

    Each target's median is divided by the target's min (so every target
    contributes on the same dimensionless scale). A layout missing from
    a target is charged that target's 95th-percentile ratio (partial
    coverage not rewarded over full coverage).

    Returns (top-k layout_ids, per_target_detail, winner_scores)."""
    per_target = {}
    for platform in PLATFORMS:
        for backend in backends:
            csv_path = csv_for(loopnest_id, platform, backend)
            if csv_path is None:
                continue
            medians = load_medians(csv_path, loopnest_id, backend,
                                   nlev, cell_dist)
            if drop_blocked:
                medians = {lid: t for lid, t in medians.items()
                           if not is_blocked_layout_id(lid)}
            if not medians:
                continue
            per_target[f"{platform}_{backend}"] = {
                "csv": str(csv_path.relative_to(EXP_DIR)),
                "medians_ms": medians,
            }

    # Per-target normalized ratios + the missing-value penalty.
    ratios = {}
    penalty = {}
    all_layouts = set()
    for tgt, blob in per_target.items():
        m = blob["medians_ms"]
        m_min = min(m.values())
        r = {lid: t / m_min for lid, t in m.items()}
        ratios[tgt] = r
        vals = sorted(r.values())
        penalty[tgt] = vals[max(0, int(0.95 * (len(vals) - 1)))] if vals else 1.0
        all_layouts.update(r)

    scores = {}
    n_targets = len(per_target)
    for lid in all_layouts:
        if not n_targets:
            continue
        log_sum = sum(math.log(ratios[t].get(lid, penalty[t])) for t in per_target)
        scores[lid] = math.exp(log_sum / n_targets)

    ranked = sorted(scores.items(), key=lambda kv: kv[1])
    winners = [lid for lid, _ in ranked[:k]]

    # Trim per-target detail to the global top-N so the JSON stays readable.
    keep = {lid for lid, _ in ranked[:max(10, k)]}
    for tgt in per_target:
        med = per_target[tgt].pop("medians_ms")
        ordered_lids = sorted(med, key=med.get)
        per_target[tgt]["n_layouts_measured"] = len(med)
        per_target[tgt]["top_medians_ms"] = {
            lid: round(med[lid], 6) for lid in keep if lid in med
        }
        per_target[tgt]["top_k"] = [
            {"layout_id": lid, "median_time_ms": round(med[lid], 6)}
            for lid in ordered_lids[:k]
        ]

    return winners, per_target, [
        {"layout_id": lid, "geomean_ratio": round(scores[lid], 4)}
        for lid in winners
    ]


# ──────────────────────────────────────────────────────────────────────
#  Array-group transfer (winners → per-group candidates)
# ──────────────────────────────────────────────────────────────────────

def load_loopnest_to_arrays():
    """loopnest_id → (chosen_nid, arrays[]) via layout_candidates.json."""
    data = json.load(open(ACCESS_JSON))
    nest_arrays = {n["nid"]: n["arrays"] for n in data["all_nests"]}
    out = {}
    for sel in data["selections"]:
        ln = sel["loopnest_id"]
        out[ln] = {
            "chosen_nid": sel["chosen_nid"],
            "class_label": sel["class_label"],
            "arrays": nest_arrays.get(sel["chosen_nid"], []),
        }
    return out


def _rank_weighted_axis_vote(touching_loopnests, loopnest_winners, axis, top_k):
    """Each touching loopnest's top-k V-winners vote for an AXIS value
    (h_first/v_first on IC, SoA/AoS on IN). Rank-1 gets `top_k` points,
    rank-2 gets `top_k-1`, ... V-ids that don't project onto this axis
    (e.g. blocked) are skipped."""
    votes = defaultdict(int)
    for ln in touching_loopnests:
        for rank, lid in enumerate(loopnest_winners.get(ln, {}).get("winners", [])):
            axis_val = project_layout_to_axis(lid, axis)
            if axis_val is None:
                continue
            votes[axis_val] += (top_k - rank)
    return votes


def _rank1_axis_union(touching_loopnests, loopnest_winners, axis):
    """Union of rank-1 axis values across every touching loopnest."""
    out = []
    seen = set()
    for ln in touching_loopnests:
        winners = loopnest_winners.get(ln, {}).get("winners", [])
        if not winners:
            continue
        axis_val = project_layout_to_axis(winners[0], axis)
        if axis_val is None or axis_val in seen:
            continue
        seen.add(axis_val)
        out.append(axis_val)
    return out


def build_array_groups(loopnest_to_arrays, loopnest_winners, top_k,
                       must_include=()):
    """Apply the canonical 5-group split (cv / ch / f / s / n). Each
    group has one storage axis it consumes (IC for cv/ch/f/s, IN for n).
    Micro-bench V-winners are PROJECTED to that axis before voting, so
    V1 and V2 are equivalent on the IC axis (both h_first), and V3..V7
    are equivalent on IC (all v_first). The IN axis splits them
    differently: V1/V3/V5/V7 = SoA, V2/V4/V6 = AoS.

    Per-group candidate selection:
      1. rank-1 axis union — each touching loopnest's top winner
         projects to an axis value; those axis values are all kept.
      2. pad with rank-weighted axis-vote winners until the candidate
         set has at least `top_k` entries.
      3. inject `must_include` axis values that have a non-zero vote.

    Group `n` has no arrays LOKI exposes (INDIRECT_ARRAYS filter), so
    no loopnest votes on the IN axis. It defaults to SoA for identity.
    """
    from _analysis_util import IC_CHOICES, IN_CHOICES
    data = json.load(open(CANONICAL_GROUPS_JSON))
    group_ids = data["group_ids"]
    canon = {g: set(data["arrays"].get(g, [])) for g in group_ids}
    labels = data["group_labels"]
    axes = data.get("group_axis", {})

    # Full choice set per axis. Used when a group has no touching
    # loopnest (LOKI can't expose its arrays, e.g. `n` whose
    # connectivity aliases are filtered): sweep both axis values so the
    # DaCe permutation search still has something to try there.
    AXIS_CHOICES = {"IC": list(IC_CHOICES), "IN": list(IN_CHOICES)}

    arr_to_lns = defaultdict(set)
    all_real_arrays = set()
    for ln_id, info in loopnest_to_arrays.items():
        for a in info["arrays"]:
            arr_to_lns[a].add(ln_id)
            all_real_arrays.add(a)

    must_include = tuple(must_include)

    groups = []
    classified = set()
    for gid in group_ids:
        arrs = sorted(canon[gid] & all_real_arrays)
        classified.update(arrs)
        touching = sorted(set().union(*(arr_to_lns[a] for a in arrs))) \
            if arrs else []
        axis = axes.get(gid, "IC")
        votes = _rank_weighted_axis_vote(touching, loopnest_winners,
                                         axis, top_k)

        # 1. rank-1 axis union — always included
        candidates = _rank1_axis_union(touching, loopnest_winners, axis)

        # 2. pad with top vote-getters on this axis
        if votes:
            ranked = sorted(votes.items(), key=lambda kv: (-kv[1], kv[0]))
            for axis_val, _ in ranked:
                if len(candidates) >= top_k:
                    break
                if axis_val not in candidates:
                    candidates.append(axis_val)

        # 3. must-include forced axis values (projected from V-ids)
        for lid in must_include:
            axis_val = project_layout_to_axis(lid, axis)
            if axis_val is None or axis_val in candidates:
                continue
            if votes.get(axis_val, 0) > 0:
                candidates.append(axis_val)

        # If no touching loopnest voted (group has 0 arrays LOKI exposed,
        # e.g. `n` connectivity), sweep the entire axis.
        if not candidates:
            candidates = AXIS_CHOICES.get(axis, [IDENTITY_LAYOUT_ID])

        groups.append({
            "group_id": gid,
            "group_label": labels.get(gid, ""),
            "axis": axis,
            "touching_loopnests": touching,
            "arrays": arrs,
            "candidate_layouts": candidates,
            "candidate_votes": {v: votes.get(v, 0) for v in candidates},
        })

    # Trailing group for anything outside the canonical taxonomy.
    missing = sorted(all_real_arrays - classified)
    if missing:
        groups.append({
            "group_id": "unclassified",
            "group_label": "arrays not in canonical permute_stage8.py split",
            "axis": "IC",
            "touching_loopnests": sorted(set().union(
                *(arr_to_lns[a] for a in missing)
            )),
            "arrays": missing,
            "candidate_layouts": ["h_first"],
            "candidate_votes": {"h_first": 0},
        })
    return groups


def crossproduct_over_groups(groups):
    """Cartesian product of candidate_layouts across every group.

    Each config is a dict `{group_id -> layout_id}`."""
    gids = [g["group_id"] for g in groups]
    axes = [g["candidate_layouts"] for g in groups]
    return [dict(zip(gids, combo)) for combo in product(*axes)]


# ──────────────────────────────────────────────────────────────────────
#  stdout summary (self-contained record per invocation)
# ──────────────────────────────────────────────────────────────────────

def print_loki_summary(groups):
    data = json.load(open(ACCESS_JSON))
    all_nests = data.get("all_nests", [])
    print()
    print(f"# 25 LOKI loop nests (from analyze_access.py):")
    print(f"  {'nid':>3}  {'shape':<6} {'ranges':<32} {'collapsed':<14} "
          f"{'arrs':>4}  class")
    for n in all_nests:
        print(f"  {n['nid']:>3}  {n.get('shape',''):<6} "
              f"{','.join(n.get('ranges', [])):<32} "
              f"{n.get('collapsed',''):<14} {n['n_arrays']:>4}  "
              f"{n.get('class_label','')}")
    print()
    print(f"# {len(groups)} array groups (from canonical_array_groups.json):")
    for g in groups:
        print(f"  {g['group_id']:<14} touching={g['touching_loopnests']} "
              f"candidates={g['candidate_layouts']}  "
              f"({len(g['arrays'])} arrays)")


# ──────────────────────────────────────────────────────────────────────
#  main
# ──────────────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--nlev", default=128, type=int,
                    help="nlev slice of the CSV to rank (default: 128 -- canonical for Tab IV)")
    ap.add_argument("--cell-dist", default="exact",
                    help="cell_dist filter ('exact', 'uniform', or '' for none)")
    ap.add_argument("-k", "--top-k", default=2, type=int,
                    help="top-k layouts retained per loopnest (default: 2)")
    ap.add_argument("--backend", default="all",
                    choices=["gpu", "cpu", "all"],
                    help="restrict ranking to GPU-only or CPU-only CSVs")
    ap.add_argument("--blocked-verdict", default=None, type=Path,
                    help="JSON from analyze_blocked_vs_unblocked.py. "
                         "Loopnests flagged 'drop blocked' exclude "
                         "B*/t*x* layouts from their winner set.")
    ap.add_argument("--must-include", default="",
                    help="comma-separated layout IDs to force into every "
                         "group's candidate set where at least one touching "
                         "loopnest voted for it (e.g. 'V6' to test the "
                         "loopnest_1 rank-1 winner across all groups it "
                         "participates in, or 'V3,V4' for vertical-first "
                         "sanity checks).")
    ap.add_argument("--out-dir", default=str(DEFAULT_OUT_DIR), type=Path,
                    help="where to write layout_crossproduct.{json,csv}")
    ap.add_argument("--no-csv", action="store_true",
                    help="skip CSV output, JSON only")
    args = ap.parse_args()

    backends = ALL_BACKENDS if args.backend == "all" else (args.backend,)
    cell_dist = args.cell_dist or None
    must_include = [s.strip() for s in args.must_include.split(",") if s.strip()]
    args.out_dir.mkdir(parents=True, exist_ok=True)

    print(f"[build_layout_crossproduct] nlev={args.nlev} "
          f"cell_dist={cell_dist} top_k={args.top_k} "
          f"backend={args.backend}"
          + (f" must_include={must_include}" if must_include else ""))

    # 1. loopnest -> (chosen_nid, arrays)
    ln_to_arrays = load_loopnest_to_arrays()

    # 2. blocked-verdict → set of loopnests that should exclude B* / t*x*
    drop_blocked_set = set()
    if args.blocked_verdict:
        vd = json.loads(args.blocked_verdict.read_text())
        drop_blocked_set = {
            int(k) for k, info in vd["verdict_per_loopnest"].items()
            if not info.get("keep_blocked", True)
        }
        print(f"  blocked-verdict: dropping blocked layouts for "
              f"loopnests {sorted(drop_blocked_set)}")

    # 3. winners per loopnest
    loopnest_winners = {}
    for ln_id in LOOPNEST_IDS:
        winners, per_target, scores = collect_winners(
            ln_id, args.nlev, cell_dist, args.top_k,
            drop_blocked=(ln_id in drop_blocked_set),
            backends=backends)
        info = ln_to_arrays.get(ln_id, {})
        loopnest_winners[ln_id] = {
            "chosen_nid": info.get("chosen_nid"),
            "class_label": info.get("class_label"),
            "arrays": info.get("arrays", []),
            "winners": winners,
            "winner_scores": scores,
            "per_target": per_target,
        }
        print(f"  loopnest_{ln_id}: winners={winners} "
              f"(from {len(per_target)} target CSVs)")

    # 4. array groups (5 canonical + optional unclassified)
    groups = build_array_groups(ln_to_arrays, loopnest_winners, args.top_k,
                                must_include=must_include)
    print(f"  array groups = {len(groups)}")

    # 5. cross product
    configs = crossproduct_over_groups(groups)
    axes_str = " x ".join(str(len(g["candidate_layouts"])) for g in groups)
    print(f"  crossproduct configs = {len(configs)}  (= {axes_str})")

    # 6. write JSON + CSV
    out = {
        "meta": {
            "nlev": args.nlev,
            "cell_dist": cell_dist,
            "top_k": args.top_k,
            "backend": args.backend,
            "n_loopnests": len(LOOPNEST_IDS),
            "n_groups": len(groups),
            "n_configs": len(configs),
            "identity_layout_id": IDENTITY_LAYOUT_ID,
        },
        "winners_per_loopnest": {str(k): v
                                  for k, v in loopnest_winners.items()},
        "array_groups": groups,
        "crossproduct": configs,
    }
    json_path = args.out_dir / "layout_crossproduct.json"
    json_path.write_text(json.dumps(out, indent=2))
    print(f"  wrote {json_path.relative_to(EXP_DIR)}")

    if not args.no_csv:
        csv_path = args.out_dir / "layout_crossproduct.csv"
        with open(csv_path, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["config_id"] + [g["group_id"] for g in groups])
            for i, cfg in enumerate(configs):
                w.writerow([i] + [cfg[g["group_id"]] for g in groups])
        print(f"  wrote {csv_path.relative_to(EXP_DIR)}")

    print_loki_summary(groups)
    return 0


if __name__ == "__main__":
    sys.exit(main())
