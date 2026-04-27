#!/usr/bin/env python3
"""
find_groups_and_loopnests.py

Report the inputs that drive the SC26 layout sweep:

  - 6 array access groups (cv, ch, f, s, n, lm) -- read from
    ``canonical_array_groups.json``. The taxonomy is fixed; this
    script is just a faithful reader.

  - 6 representative loopnests -- *manually pinned* in
    ``select_loopnests.py`` (loopnest_1) plus the auto-selected
    cohorts. Kernel names are read from each
    ``loopnest_<i>/plot_paper.py``'s ``KERNEL`` constant.

Usage:
    python access_analysis/find_groups_and_loopnests.py
    python access_analysis/find_groups_and_loopnests.py --out groups_and_loopnests.json
"""

import argparse
import json
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
EXP_DIR = HERE.parent
CANONICAL_GROUPS_JSON = HERE / "canonical_array_groups.json"


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


_KERNEL_RE = re.compile(r'^KERNEL\s*=\s*"([^"]+)"', re.MULTILINE)


def kernel_for(loopnest_id):
    p = EXP_DIR / f"loopnest_{loopnest_id}" / "plot_paper.py"
    if not p.is_file():
        return None
    m = _KERNEL_RE.search(p.read_text())
    return m.group(1) if m else None


# Manually chosen via select_loopnests.py (one pinned, five auto-picked
# by tiebreaker). Kept in sync with chosen_loopnests.md.
LOOPNEST_LABELS = {
    1: "indirect stencil, full vertical",
    2: "direct stencil, partial vertical",
    3: "direct stencil, full vertical",
    4: "indirect stencil, partial vertical",
    5: "horizontal-only (boundary)",
    6: "vertical-only (level reduction)",
}


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--out", type=Path, default=HERE / "groups_and_loopnests.json")
    args = ap.parse_args()

    groups = load_groups()
    loopnests = []
    for ln_id, label in LOOPNEST_LABELS.items():
        loopnests.append({
            "loopnest_id": ln_id,
            "kernel": kernel_for(ln_id),
            "class_label": label,
            "results_dir": str((EXP_DIR / f"loopnest_{ln_id}" / "results")
                               .relative_to(EXP_DIR)),
        })

    print(f"Access groups: {len(groups)}")
    for g in groups:
        print(f"  {g['group_id']:<3} axis={g['axis']:<3} "
              f"n_arrays={g['n_arrays']:<3} -- {g['label']}")
    print()
    print(f"Loopnests (manually chosen, see select_loopnests.py): {len(loopnests)}")
    for ln in loopnests:
        print(f"  loopnest_{ln['loopnest_id']:<2} kernel={ln['kernel']:<20} "
              f"-- {ln['class_label']}")

    args.out.write_text(json.dumps({
        "access_groups": groups,
        "loopnests": loopnests,
        "loopnest_selection_source": "select_loopnests.py (manually pinned + auto-tiebreak)",
    }, indent=2))
    print(f"\nWrote {args.out.relative_to(EXP_DIR)}")


if __name__ == "__main__":
    main()
