#!/usr/bin/env python3
"""
generate_canonical_groups.py

Derive the 5-group layout taxonomy (cv / ch / f / s / n) that
`../build_layout_crossproduct.py` feeds into the full velocity sweep.

Source of truth
---------------
Every array name + its touching loop nests come from LOKI via
`layout_candidates.json` (produced by `select_loopnests.py`, which in
turn wraps `analyze_access.py`). Nothing is hand-transcribed except the
connectivity alias list (see below).

Classification rules
--------------------
  n  (CONN)
      Every array name matching the Fortran indirection pattern
        p_patch%{cells,edges,verts}%(edge|cell|vertex|neighbor|quad)_(idx|blk)
      PLUS the local-alias names ICON uses inside the kernel body
      (icidx, icblk, ividx, ivblk, iqidx, iqblk, incidx, incblk, ieidx,
      ieblk). LOKI strips these aliases when building per-nest array
      lists (see coaccess.py::INDIRECT_ARRAYS), so we force them
      manually; they are still the arrays the downstream sweep
      permutes, so the `n` axis must be emitted even when empty.

  f  (FIELDS)
      Any array whose qualified name begins with `p_prog%` or `p_diag%`.

  s  (STENCIL)
      Read-only metric / interp / edge scalars — qualified names that
      begin with `p_metrics%`, `p_int%`, `p_patch%edges%` (non-`_idx` /
      non-`_blk`), or `p_patch%cells%` (non-`_idx` / non-`_blk`).

  cv / ch  (COMPUTE_VERT / COMPUTE_HORIZ)
      Work arrays (`z_*`, `cfl_*`, `lev*mask`). Distinguished by the
      loop role of the outer-producing nest the array shows up in:

        * cv -> at least one touching nest has loop-shape `v.h` with
                full_vert range (jk-outer compute)
        * ch -> every touching nest has loop-shape `h` (boundary
                sweep) or `v.h` with partial_vert range (horizontal
                stencil)

      This matches the intent of permute_stage8.py's split:
      `_COMPUTE_VERT_PERMUTED` contains arrays written in jk-outer
      producer loops, `_COMPUTE_HORIZ_PERMUTED` contains the
      horizontal-stencil producers.

Output
------
Writes canonical_array_groups.json alongside this script, with:

  {
    "source":      "derived from layout_candidates.json + CONN alias list",
    "group_ids":   ["cv","ch","f","s","n"],
    "group_labels":{...},
    "arrays":      {"cv":[...], "ch":[...], "f":[...], "s":[...], "n":[...]},
    "unclassified":[...]    # surfaced so you notice regressions
  }

Usage
-----
  python3 generate_canonical_groups.py
  python3 generate_canonical_groups.py --loki-json layout_candidates.json \\
                                       --out canonical_array_groups.json
"""

import argparse
import json
import re
from collections import defaultdict
from pathlib import Path

# --- Connectivity arrays LOKI cannot see: hardcoded. -----------------------
CONN_ALIASES = [
    # Local aliases inside the kernel body (LOKI strips these).
    "icidx", "icblk",
    "ieidx", "ieblk",
    "ividx", "ivblk",
    "iqidx", "iqblk",
    "incidx", "incblk",
]

# DaCe/fully-qualified connectivity names that also belong to `n` if the
# upstream tool reports them.
CONN_QUAL_RE = re.compile(
    r"^p_patch%(cells|edges|verts)%"
    r"(edge|cell|vertex|neighbor|quad)_(idx|blk)$"
)

GROUP_LABELS = {
    "cv": "COMPUTE_VERT   (2D/3D work arrays written in jk-outer loops)",
    "ch": "COMPUTE_HORIZ  (work arrays written by horizontal stencils)",
    "f":  "FIELDS         (prognostic + diagnostic state)",
    "s":  "STENCIL        (read-only metric + interp coefficients)",
    "n":  "CONN           (edge/vertex/cell index connectivity tables)",
}

# Which storage axis each group consumes. cv/ch/f/s are (h,v,b) arrays —
# the layout choice that matters is whether `je` or `jk` is innermost
# (IC axis). `n` is (N,2) connectivity — the choice is SoA vs AoS (IN
# axis). Each V-id in the micro-bench encodes BOTH axes, but each group
# only consumes the axis relevant to its arrays.
GROUP_AXIS = {
    "cv": "IC",
    "ch": "IC",
    "f":  "IC",
    "s":  "IC",
    "n":  "IN",
}


def _is_conn(name: str) -> bool:
    if name in CONN_ALIASES:
        return True
    return bool(CONN_QUAL_RE.match(name))


def _is_field(name: str) -> bool:
    return name.startswith("p_prog%") or name.startswith("p_diag%")


def _is_stencil(name: str) -> bool:
    if name.startswith("p_metrics%") or name.startswith("p_int%"):
        return True
    if name.startswith("p_patch%edges%") or name.startswith("p_patch%cells%"):
        # Exclude connectivity — already handled by _is_conn.
        return not _is_conn(name)
    return False


def _cv_or_ch(name: str, touching_nests_info) -> str:
    """touching_nests_info is a list of dicts each with keys 'shape' and
    'ranges' (LOKI output format from layout_candidates.json::all_nests)."""
    for nest in touching_nests_info:
        shape = nest.get("shape", "")
        ranges = nest.get("ranges", [])
        if shape.startswith("v.") and "full_vert" in ranges:
            return "cv"
    return "ch"


def classify(name: str, touching_nests_info) -> str:
    if _is_conn(name):
        return "n"
    if _is_field(name):
        return "f"
    if _is_stencil(name):
        return "s"
    return _cv_or_ch(name, touching_nests_info)


def build(loki_json_path: Path) -> dict:
    data = json.loads(loki_json_path.read_text())
    nests_by_id = {n["nid"]: n for n in data["all_nests"]}

    # arr -> set of nest ids touching it
    arr_to_nests = defaultdict(set)
    for nest in data["all_nests"]:
        for a in nest.get("arrays", []):
            arr_to_nests[a].add(nest["nid"])

    groups = {g: [] for g in ("cv", "ch", "f", "s", "n")}
    unclassified = []

    # 1. Derived: every array LOKI reports.
    for arr, nids in arr_to_nests.items():
        touching = [nests_by_id[nid] for nid in sorted(nids)]
        g = classify(arr, touching)
        if g in groups:
            groups[g].append(arr)
        else:
            unclassified.append(arr)

    # 2. Forced: connectivity aliases LOKI cannot expose.
    for alias in CONN_ALIASES:
        if alias not in groups["n"]:
            groups["n"].append(alias)

    for g in groups:
        groups[g].sort()
    unclassified.sort()

    return {
        "source": "derived from layout_candidates.json + CONN alias list "
                  "(local-alias connectivity names are hardcoded because "
                  "LOKI's INDIRECT_ARRAYS filter strips them from per-nest "
                  "array lists).",
        "group_ids": ["cv", "ch", "f", "s", "n"],
        "group_labels": GROUP_LABELS,
        "group_axis":   GROUP_AXIS,
        "arrays": groups,
        "unclassified": unclassified,
    }


def main():
    here = Path(__file__).resolve().parent
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--loki-json", type=Path,
                    default=here / "layout_candidates.json",
                    help="LOKI output produced by select_loopnests.py "
                         "(default: ./layout_candidates.json)")
    ap.add_argument("--out", type=Path,
                    default=here / "canonical_array_groups.json",
                    help="destination JSON "
                         "(default: ./canonical_array_groups.json)")
    args = ap.parse_args()

    out = build(args.loki_json)

    args.out.write_text(json.dumps(out, indent=2) + "\n")

    print(f"[generate_canonical_groups] read {args.loki_json.name}")
    for g in out["group_ids"]:
        print(f"  {g:<3}  {len(out['arrays'][g]):>3} arrays  "
              f"{GROUP_LABELS[g]}")
    if out["unclassified"]:
        print(f"  [unclassified]  {len(out['unclassified'])} arrays: "
              f"{out['unclassified']}")
    print(f"  wrote {args.out.name}")


if __name__ == "__main__":
    main()
