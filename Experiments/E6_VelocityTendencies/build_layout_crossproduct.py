#!/usr/bin/env python3
"""
build_layout_crossproduct.py

Prepare the input list for the full velocity-tendencies sweep, following the
recipe in section IV-A of the paper ("Searching the Transformation Space"):

  Step 3-4: per representative loop nest, pick the top-k (=2) winning layouts
            from the measured sweep (CSV under loopnest_{1..6}/results/*/).

  Step 5:   take the Cartesian product of those winners across all array
            groups, plus an identity layout for every array group that no
            representative loop nest covered.

Input sources
-------------
  access_analysis/layout_candidates.json
      chosen_nid / arrays per representative loop nest (nid 1..6 -> N arrays)

  loopnest_{1..6}/results/{beverin,daint}/<kernel>_{cpu,gpu}.csv
      benchmarked sweep. Two schemas:
        - loopnest_1:         variant, blocking, parallelization, ...
        - loopnest_{2..6}:    V, layout, schedule, ...   (CPU)
                              V, config_label, run_label (GPU)

Output
------
  full_velocity_tendencies/layout_crossproduct.json
      {
        "winners_per_loopnest": { "1": {...}, ..., "6": {...} },
        "array_groups":          [ {group_id, arrays, touching_loopnests,
                                    candidate_layouts:[...]} ],
        "crossproduct":          [ { group_id -> layout_id }, ... ],
        "meta":                  { nlev, cell_dist, k, n_configs, ... }
      }

  full_velocity_tendencies/layout_crossproduct.csv
      one row per config, columns = group_id_0..group_id_{G-1}, values =
      layout_id chosen for that group.
"""

import argparse
import csv
import json
import os
import statistics
import sys
from collections import defaultdict
from itertools import product
from pathlib import Path

EXP_DIR = Path(__file__).resolve().parent
ACCESS_JSON = EXP_DIR / "access_analysis" / "layout_candidates.json"
CANONICAL_GROUPS_JSON = EXP_DIR / "access_analysis" / "canonical_array_groups.json"
DEFAULT_OUT_DIR = EXP_DIR / "full_velocity_tendencies"

LOOPNEST_IDS = [1, 2, 3, 4, 5, 6]
PLATFORMS = ("beverin", "daint")
ALL_BACKENDS = ("cpu", "gpu")

# Identity layout: no permutation, no blocking. Matches the convention used
# elsewhere in this repo where V=1 is horizontal-first row-major.
IDENTITY_LAYOUT_ID = "V1"


def _csv_for(loopnest_id, platform, backend):
    """Return the one per-kernel CSV for (loopnest, platform, backend)."""
    d = EXP_DIR / f"loopnest_{loopnest_id}" / "results" / platform
    if not d.is_dir():
        return None
    for p in sorted(d.glob(f"*_{backend}.csv")):
        name = p.name
        if name.startswith("metrics"):
            continue
        if name.endswith("_old.csv"):
            continue
        return p
    return None


def _layout_id_l1_cpu(row):
    """loopnest_1 CPU schema: variant + blocking columns.

    variant >= 1 and blocking == 0  ->  "V{variant}"   (unblocked permutation)
    variant == 0 and blocking  > 0  ->  "B{blocking}"  (blocked layout)
    """
    V = int(row["variant"])
    B = int(row["blocking"])
    if V > 0 and B == 0:
        return f"V{V}"
    if V == 0 and B > 0:
        return f"B{B}"
    # Fallback — defensive, should not happen in clean data.
    return f"V{V}_B{B}"


def _layout_id_l1_gpu(row):
    """loopnest_1 GPU schema: only `variant` distinguishes layouts (the
    config_label prefix encodes the thread-block schedule, which varies
    WITHIN a layout). All V=0 rows share the blocked-row-major layout; V>=1
    are unblocked permutations."""
    V = int(row["variant"])
    return f"V{V}"


def _layout_id_new_cpu(row):
    """loopnest_{2..6} CPU schema: explicit `layout` column (e.g. V1, B64).

    A few rows contain a bare integer (8/16/32/64) — treat those as a
    blocked layout "B{n}" since the factor matches the blocking family.
    """
    lay = row["layout"]
    if lay.isdigit():
        return f"B{lay}"
    return lay


def _layout_id_new_gpu(row):
    """loopnest_{2..6} GPU schema: no `layout` column — parse config_label
    prefix. Examples:
       "B128_bx128_by01"  -> blocked layout, factor 128   -> "B128"
       "t08x08"           -> tile-only unblocked          -> "V{V}"
    """
    cfg = row["config_label"]
    if cfg.startswith("B"):
        return cfg.split("_", 1)[0]
    V = int(row["V"])
    return f"V{V}"


def extract_layout_id(row, schema, backend):
    """Dispatch to the right per-schema layout extractor."""
    if schema == "l1_cpu":
        return _layout_id_l1_cpu(row)
    if schema == "l1_gpu":
        return _layout_id_l1_gpu(row)
    if backend == "cpu":
        return _layout_id_new_cpu(row)
    return _layout_id_new_gpu(row)


def detect_schema(header, loopnest_id, backend):
    """Return one of: 'l1_cpu', 'l1_gpu', 'new_cpu', 'new_gpu'.

    L1 uses `variant`; L{2..6} use `V`. Blocking is only a separate column
    on the L1 CPU side; L1 GPU collapses it into config_label so we treat
    every V=0 row as the single blocked layout."""
    if "variant" in header:
        return "l1_cpu" if backend == "cpu" else "l1_gpu"
    if "V" in header:
        return "new_cpu" if backend == "cpu" else "new_gpu"
    raise RuntimeError(
        f"loopnest_{loopnest_id} ({backend}): unknown CSV schema {list(header)}"
    )


def load_medians(csv_path, loopnest_id, backend, nlev, cell_dist):
    """Return { layout_id : median(time_ms) } — the median is taken over
    every (schedule, config_label, run_id) sample for that layout_id.
    Filtered to nlev and cell_dist.
    """
    rows = list(csv.DictReader(open(csv_path)))
    if not rows:
        return {}
    schema = detect_schema(rows[0].keys(), loopnest_id, backend)
    nlev_str = str(nlev)

    # Collect all times per layout_id, then median.
    bucket = defaultdict(list)
    for r in rows:
        if r.get("nlev") != nlev_str:
            continue
        if "cell_dist" in r and cell_dist and r["cell_dist"] != cell_dist:
            continue
        try:
            lid = extract_layout_id(r, schema, backend)
        except (KeyError, ValueError):
            continue
        try:
            t = float(r["time_ms"])
        except (KeyError, ValueError):
            continue
        bucket[lid].append(t)

    return {lid: statistics.median(ts) for lid, ts in bucket.items() if ts}


def top_k_winners(medians, k):
    """Lowest-median k layout ids."""
    return [lid for lid, _ in sorted(medians.items(), key=lambda kv: kv[1])[:k]]


def _is_blocked_layout_id(lid):
    """B{n} or t{TX}x{TY} prefixes — every blocked/tiled-storage family."""
    return lid.startswith("B") or lid.startswith("t")


def collect_winners(loopnest_id, nlev, cell_dist, k, drop_blocked=False,
                    backends=ALL_BACKENDS):
    """Rank layouts per loopnest by the GEOMEAN of normalized medians across
    every measured {platform, backend} target. `backends` restricts which
    device families contribute to the ranking — pass ("gpu",) for GPU-only
    or ("cpu",) for CPU-only sweeps.

    Normalization: each target's median is divided by the target's min so
    every target contributes on the same dimensionless scale. A layout that
    is missing from a target is charged the target's 95th-percentile ratio
    (so partial coverage is not rewarded over full coverage).

    Returns top-k layout_ids plus a per-target detail block for
    provenance/debugging.
    """
    per_target = {}
    all_targets = []
    for platform in PLATFORMS:
        for backend in backends:
            csv_path = _csv_for(loopnest_id, platform, backend)
            if csv_path is None:
                continue
            medians = load_medians(csv_path, loopnest_id, backend,
                                   nlev, cell_dist)
            if drop_blocked:
                medians = {lid: t for lid, t in medians.items()
                           if not _is_blocked_layout_id(lid)}
            if not medians:
                continue
            tgt_key = f"{platform}_{backend}"
            per_target[tgt_key] = {
                "csv": str(csv_path.relative_to(EXP_DIR)),
                "n_layouts_measured": len(medians),
                "medians_ms": medians,
            }
            all_targets.append((tgt_key, medians))

    # Build per-target normalized ratios and the missing-value penalty.
    target_ratios = {}
    target_penalty = {}
    all_layouts = set()
    for tgt_key, medians in all_targets:
        m_min = min(medians.values())
        ratios = {lid: (t / m_min) for lid, t in medians.items()}
        target_ratios[tgt_key] = ratios
        # 95th percentile of present ratios is the "you weren't measured here"
        # penalty — punishes incomplete coverage without making it infinite.
        vals = sorted(ratios.values())
        idx = max(0, int(0.95 * (len(vals) - 1)))
        target_penalty[tgt_key] = vals[idx] if vals else 1.0
        all_layouts.update(ratios.keys())

    scores = {}
    for lid in all_layouts:
        log_sum = 0.0
        n = 0
        for tgt_key, _ in all_targets:
            r = target_ratios[tgt_key].get(lid, target_penalty[tgt_key])
            import math
            log_sum += math.log(r)
            n += 1
        scores[lid] = math.exp(log_sum / n) if n else float("inf")

    ranked = sorted(scores.items(), key=lambda kv: kv[1])
    winners = [lid for lid, _ in ranked[:k]]

    # Tighten per-target detail: keep only the layouts that are in the
    # global top-N (10) so the JSON stays readable.
    keep = {lid for lid, _ in ranked[:max(10, k)]}
    for tgt_key in per_target:
        med = per_target[tgt_key].pop("medians_ms")
        per_target[tgt_key]["top_medians_ms"] = {
            lid: round(med[lid], 6) for lid in keep if lid in med
        }
        per_target[tgt_key]["top_k"] = [
            {"layout_id": lid,
             "median_time_ms": round(med[lid], 6)}
            for lid in top_k_winners(med, k)
        ]

    winner_scores = [
        {"layout_id": lid, "geomean_ratio": round(scores[lid], 4)}
        for lid in winners
    ]
    return winners, per_target, winner_scores


def load_loopnest_to_arrays():
    """loopnest_id -> (chosen_nid, [arrays])  via layout_candidates.json +
    all_nests lookup."""
    data = json.load(open(ACCESS_JSON))
    nest_arrays = {n["nid"]: n["arrays"] for n in data["all_nests"]}
    out = {}
    for sel in data["selections"]:
        ln_id = sel["loopnest_id"]
        nid = sel["chosen_nid"]
        out[ln_id] = {
            "chosen_nid": nid,
            "class_label": sel["class_label"],
            "arrays": nest_arrays.get(nid, []),
        }
    return out


def build_array_groups_canonical(loopnest_to_arrays, loopnest_winners,
                                 top_k):
    """Canonical 5-group split from icon-artifacts/velocity/sc26_layout/
    permute_stage8.py (cv, ch, f, s, n). Every array named in the JSON is
    assigned to its fixed group; candidate layouts per group are the
    rank-weighted vote from every touching loopnest's top-k (same as the
    auto-derived path, just with pinned group membership).

    Arrays not listed in canonical_array_groups.json are collected into a
    single synthetic group `unclassified` with identity layout only.
    """
    data = json.load(open(CANONICAL_GROUPS_JSON))
    group_ids = data["group_ids"]
    canon = {g: set(data["arrays"].get(g, [])) for g in group_ids}
    labels = data["group_labels"]

    # Reverse: array -> set of loopnests touching it
    arr_to_lns = defaultdict(set)
    all_real_arrays = set()
    for ln_id, info in loopnest_to_arrays.items():
        for a in info["arrays"]:
            arr_to_lns[a].add(ln_id)
            all_real_arrays.add(a)

    groups = []
    classified = set()
    for gid in group_ids:
        arrs_in_group = sorted(canon[gid] & all_real_arrays)
        classified.update(arrs_in_group)
        touching = sorted(set().union(
            *(arr_to_lns[a] for a in arrs_in_group)
        )) if arrs_in_group else []
        votes = defaultdict(int)
        for ln_id in touching:
            wlist = loopnest_winners.get(ln_id, {}).get("winners", [])
            for rank, lid in enumerate(wlist):
                votes[lid] += (top_k - rank)
        if votes:
            ranked = sorted(votes.items(),
                            key=lambda kv: (-kv[1], kv[0]))
            candidates = [lid for lid, _ in ranked[:top_k]]
        else:
            candidates = [IDENTITY_LAYOUT_ID]
        groups.append({
            "group_id": gid,
            "group_label": labels.get(gid, ""),
            "touching_loopnests": touching,
            "arrays": arrs_in_group,
            "candidate_layouts": candidates,
            "candidate_votes": {lid: votes.get(lid, 0)
                                 for lid in candidates},
        })

    # Anything in the LOKI analysis not covered by the canonical split
    # goes to a trailing "unclassified" group, pinned to identity so it
    # never expands the cross-product.
    missing = sorted(all_real_arrays - classified)
    if missing:
        groups.append({
            "group_id": "unclassified",
            "group_label": "arrays not in canonical permute_stage8.py split",
            "touching_loopnests": sorted(set().union(
                *(arr_to_lns[a] for a in missing)
            )),
            "arrays": missing,
            "candidate_layouts": [IDENTITY_LAYOUT_ID],
            "candidate_votes": {IDENTITY_LAYOUT_ID: 0},
        })
    return groups


def build_array_groups(loopnest_to_arrays, loopnest_winners, top_k):
    """Group arrays by the *set of loopnests that touches them* (equivalence
    classes from the paper's §III-F). For each group, the candidate-layout
    set is capped at `top_k`, scored by a rank-weighted vote across every
    touching loopnest: rank-1 winner contributes `top_k` points, rank-2
    contributes `top_k-1`, and so on.

    Arrays touched by no measured loopnest form groups whose only candidate
    is IDENTITY_LAYOUT_ID.
    """
    # arrays -> set of loopnest_ids that touch it
    arr_to_lns = defaultdict(set)
    for ln_id, info in loopnest_to_arrays.items():
        for a in info["arrays"]:
            arr_to_lns[a].add(ln_id)

    # inverse: frozenset(loopnest_ids) -> [arrays]
    class_to_arrays = defaultdict(list)
    for a, lns in arr_to_lns.items():
        class_to_arrays[frozenset(lns)].append(a)

    groups = []
    for i, (lns_key, arrs) in enumerate(
            sorted(class_to_arrays.items(),
                   key=lambda kv: (sorted(kv[0]), kv[1]))):
        touching = sorted(lns_key)
        votes = defaultdict(int)
        for ln_id in touching:
            wlist = loopnest_winners.get(ln_id, {}).get("winners", [])
            for rank, lid in enumerate(wlist):
                votes[lid] += (top_k - rank)
        if votes:
            ranked = sorted(votes.items(),
                            key=lambda kv: (-kv[1], kv[0]))
            candidates = [lid for lid, _ in ranked[:top_k]]
        else:
            candidates = [IDENTITY_LAYOUT_ID]
        groups.append({
            "group_id": f"G{i}",
            "touching_loopnests": touching,
            "arrays": sorted(arrs),
            "candidate_layouts": candidates,
            "candidate_votes": {lid: votes.get(lid, 0)
                                 for lid in candidates},
        })
    return groups


def crossproduct_over_groups(groups):
    """Cartesian product: one layout choice per group, covering every
    combination of candidate_layouts."""
    gids = [g["group_id"] for g in groups]
    axes = [g["candidate_layouts"] for g in groups]
    configs = []
    for combo in product(*axes):
        configs.append(dict(zip(gids, combo)))
    return configs


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--nlev", default=96, type=int,
                    help="nlev slice of the CSV to rank (default: 96)")
    ap.add_argument("--cell-dist", default="exact",
                    help="cell_dist filter ('exact', 'uniform', or '' for none)")
    ap.add_argument("-k", "--top-k", default=2, type=int,
                    help="top-k layouts to retain per loopnest per target "
                         "(default: 2 per the paper)")
    ap.add_argument("--out-dir", default=str(DEFAULT_OUT_DIR), type=Path,
                    help="where to write layout_crossproduct.{json,csv}")
    ap.add_argument("--no-csv", action="store_true",
                    help="skip CSV output, JSON only")
    ap.add_argument("--blocked-verdict", default=None, type=Path,
                    help="JSON produced by analyze_blocked_vs_unblocked.py. "
                         "Loopnests flagged 'drop blocked' have B*/t*x* "
                         "layouts excluded from their winner set before "
                         "top-k selection, shrinking the crossproduct.")
    ap.add_argument("--backend", default="all",
                    choices=["gpu", "cpu", "all"],
                    help="restrict ranking to GPU-only or CPU-only CSVs. "
                         "Default 'all' uses every measured target.")
    ap.add_argument("--canonical-groups", action="store_true", default=True,
                    help="use the 5-group canonical split from "
                         "access_analysis/canonical_array_groups.json "
                         "(default). Matches the real sweep in "
                         "icon-artifacts/velocity/sc26_layout/"
                         "permute_stage8.py.")
    ap.add_argument("--equivalence-groups", dest="canonical_groups",
                    action="store_false",
                    help="fall back to per-loopnest-membership "
                         "equivalence-class grouping (11 groups).")
    args = ap.parse_args()
    backends = ALL_BACKENDS if args.backend == "all" else (args.backend,)

    cell_dist = args.cell_dist or None
    args.out_dir.mkdir(parents=True, exist_ok=True)

    print(f"[build_layout_crossproduct] nlev={args.nlev} "
          f"cell_dist={cell_dist} top_k={args.top_k} "
          f"backend={args.backend}")
    print(f"  access_json = {ACCESS_JSON.relative_to(EXP_DIR)}")

    # 1. loopnest -> (chosen_nid, arrays)
    ln_to_arrays = load_loopnest_to_arrays()

    drop_blocked_set = set()
    if args.blocked_verdict:
        with open(args.blocked_verdict) as f:
            vd = json.load(f)
        for ln_id_str, info in vd["verdict_per_loopnest"].items():
            if not info.get("keep_blocked", True):
                drop_blocked_set.add(int(ln_id_str))
        print(f"  blocked-verdict: dropping blocked layouts for "
              f"loopnests {sorted(drop_blocked_set)}")

    # 2. winners per loopnest
    loopnest_winners = {}
    for ln_id in LOOPNEST_IDS:
        winners, per_target, winner_scores = collect_winners(
            ln_id, args.nlev, cell_dist, args.top_k,
            drop_blocked=(ln_id in drop_blocked_set),
            backends=backends)
        info = ln_to_arrays.get(ln_id, {})
        loopnest_winners[ln_id] = {
            "chosen_nid": info.get("chosen_nid"),
            "class_label": info.get("class_label"),
            "arrays": info.get("arrays", []),
            "winners": winners,
            "winner_scores": winner_scores,
            "per_target": per_target,
        }
        print(f"  loopnest_{ln_id}: winners={winners} "
              f"(from {len(per_target)} target CSVs)")

    # 3. array groups + candidate layouts
    if args.canonical_groups:
        groups = build_array_groups_canonical(ln_to_arrays, loopnest_winners,
                                              args.top_k)
    else:
        groups = build_array_groups(ln_to_arrays, loopnest_winners,
                                    args.top_k)
    n_unconsidered = sum(1 for g in groups
                         if g["candidate_layouts"] == [IDENTITY_LAYOUT_ID]
                         and not g["touching_loopnests"])
    print(f"  array groups = {len(groups)}  "
          f"(unconsidered: {n_unconsidered})")

    # 4. cross product
    configs = crossproduct_over_groups(groups)
    print(f"  crossproduct configs = {len(configs)}  "
          f"(= {' x '.join(str(len(g['candidate_layouts'])) for g in groups)})")

    # 5. write JSON
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
    with open(json_path, "w") as f:
        json.dump(out, f, indent=2)
    print(f"  wrote {json_path.relative_to(EXP_DIR)}")

    # 6. write CSV (flat, one row per config)
    if not args.no_csv:
        csv_path = args.out_dir / "layout_crossproduct.csv"
        header = ["config_id"] + [g["group_id"] for g in groups]
        with open(csv_path, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(header)
            for i, cfg in enumerate(configs):
                w.writerow([i] + [cfg[g["group_id"]] for g in groups])
        print(f"  wrote {csv_path.relative_to(EXP_DIR)}")

    # 7. LOKI-sourced summary — every loop nest and every array group,
    #    so the crossproduct is readable without opening the JSON.
    print_loki_summary(json.load(open(ACCESS_JSON)), groups)
    return 0


def print_loki_summary(access_data, groups):
    """Echo the upstream LOKI analysis (25 loop nests, their pattern
    signatures, and the array groups we derived from them) so every
    invocation of this script produces a self-contained record of which
    arrays are driven by which loop nests and which layout candidates."""
    all_nests = access_data.get("all_nests", [])
    print()
    print("# ── LOKI loop nests (velocity_tendencies, R02B05) ────────────")
    print(f"#   {len(all_nests)} nests extracted by access_analysis/"
          f"analyze_access.py from")
    print(f"#   velocity_advection_preprocessed.f90")
    print(f"{'nid':>3}  {'shape':<6} {'ranges':<32} {'collapsed':<14}  "
          f"{'arrs':>4}  class")
    print("  " + "-" * 82)
    for n in all_nests:
        rr = ",".join(n.get("ranges", []))
        print(f"{n['nid']:>3}  {n.get('shape',''):<6} {rr:<32} "
              f"{n.get('collapsed',''):<14}  "
              f"{n['n_arrays']:>4}  {n.get('class_label','')}")

    print()
    print("# ── Array groups (equivalence classes on loopnest membership) ")
    print(f"#   {len(groups)} groups. Candidate layouts are rank-weighted")
    print(f"#   votes from touching loopnests' top-k (see winners above).")
    for g in groups:
        print(f"  {g['group_id']:<4} touching=loopnest{tuple(g['touching_loopnests'])}  "
              f"candidates={g['candidate_layouts']}  "
              f"({len(g['arrays'])} arrays)")
        for a in g["arrays"]:
            print(f"        {a}")


if __name__ == "__main__":
    sys.exit(main())
