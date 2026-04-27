#!/usr/bin/env python3
"""
list_v123_configs.py -- enumerate the V_k cross-product over E6's
layout groups (read from
``Experiments/E6_VelocityTendencies/access_analysis/layout_groups.json``)
and print the unique permute-map signatures that would result.

Pure stdlib; no DaCe / matplotlib / etc. Safe to run on a login node.
This is the JSON-parsing companion to ``run_stage8_permutations.py
--dry-run``: that one previews configs already registered in
``PERMUTE_CONFIGS``; this one previews configs that *would* be
generated from the V_k spec for a 3^N cross-product over the N
groups, before any module loads.

Usage:
    python list_v123_configs.py
    python list_v123_configs.py --vks V1,V2,V6
    python list_v123_configs.py --json /path/to/layout_groups.json
"""

import argparse
import json
from collections import OrderedDict
from itertools import product
from pathlib import Path


_E6_DIR = Path(__file__).resolve().parent.parent / "E6_VelocityTendencies"
DEFAULT_JSON = _E6_DIR / "access_analysis" / "layout_groups.json"


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--json", type=Path, default=DEFAULT_JSON,
                    help=f"layout_groups.json path (default: {DEFAULT_JSON})")
    ap.add_argument("--vks", default="V1,V2,V6",
                    help="comma-separated V_k IDs to cross-product (default: V1,V2,V6)")
    ap.add_argument("--names-only", action="store_true",
                    help="print just the unique config names (one per line, no header)")
    args = ap.parse_args()

    spec = json.loads(args.json.read_text())
    groups = spec["groups"]
    vks = [v.strip() for v in args.vks.split(",")]

    if not args.names_only:
        print(f"Source : {args.json}")
        print(f"Groups : {len(groups)}  -> {[g['name'] for g in groups]}")
        print(f"V_k    : {vks}")
        print()
        print(f"{'group':<6} {'rank':>4} {'arrays':>6}  V_k perms")
        print("-" * 70)
        for g in groups:
            perms = "  ".join(f"{v}={g['vk_perms'].get(v)}" for v in vks)
            collapse = ""
            v1, v2 = g['vk_perms'].get('V1'), g['vk_perms'].get('V2')
            if v1 == v2 and 'V1' in vks and 'V2' in vks:
                collapse = "  (V1==V2)"
            print(f"{g['name']:<6} {g['rank']:>4} {len(g['arrays']):>6}  {perms}{collapse}")
        print()

    # Enumerate the raw cross-product, dedupe by per-array permute_map
    # signature. Two assignments collapse iff every array gets the same
    # final permutation (which happens whenever V1==V2 for a group).
    raw_count = len(vks) ** len(groups)
    seen_sigs: "OrderedDict[tuple, tuple[str,int]]" = OrderedDict()
    dupe_count = 0

    for assignment in product(vks, repeat=len(groups)):
        permute_map = {}
        for g, v in zip(groups, assignment):
            perm = g["vk_perms"].get(v)
            if perm is None:
                continue
            for arr in g["arrays"]:
                permute_map[arr] = perm
        sig = tuple(sorted((a, tuple(p)) for a, p in permute_map.items()))
        cfg_name = "v123_" + "_".join(f"{g['name']}-{v}" for g, v in zip(groups, assignment))
        if sig in seen_sigs:
            dupe_count += 1
            continue
        seen_sigs[sig] = (cfg_name, len(permute_map))

    if args.names_only:
        for cfg_name, _ in seen_sigs.values():
            print(cfg_name)
        return

    print(f"Raw cells       : {len(vks)}^{len(groups)} = {raw_count}")
    print(f"Unique configs  : {len(seen_sigs)}  (dropped {dupe_count} duplicates from V1==V2 collapse)")
    print()
    print(f"{'#':>4}  {'config_name':<60} {'arrays':>6}")
    print("-" * 78)
    for i, (cfg_name, n_arrays) in enumerate(seen_sigs.values(), 1):
        print(f"{i:>4}  {cfg_name:<60} {n_arrays:>6}")
    print()
    print(f"Sweep size: {len(seen_sigs)} configs * 2 (shuffled/unshuffled) "
          f"= {len(seen_sigs) * 2} binaries.")


if __name__ == "__main__":
    main()
