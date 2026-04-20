#!/usr/bin/env python3
"""
select_loopnests.py — deterministic per-pattern-class selection of
representative loop nests from velocity_tendencies.

Reads velocity_advection_preprocessed.analysis.md (produced by
analyze_access.py), lists every nest in every pattern class, applies
a fixed tiebreaker, and writes:

  - layout_candidates.json   structured: all candidates + chosen
  - chosen_loopnests.md      human-readable summary

Pinned overrides:
  loopnest_1 ↔ Nest 6 (z_v_grad_w producer, class h:U v:S full_vert).
  Anything pinned wins over the heuristic for that class.

Tiebreaker (applied in order):
  1. widest nest      — most unique array names (descending)
  2. full_vert > partial_vert  (full range is more general)
  3. lowest source-order nest id  (reproducible)

Usage:
  python select_loopnests.py
  python select_loopnests.py --analysis path/to/analysis.md
"""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass, field, asdict
from pathlib import Path
from collections import defaultdict


HERE = Path(__file__).resolve().parent
DEFAULT_ANALYSIS = HERE / "velocity_advection_preprocessed.analysis.md"

# loopnest_id → (nest_id, class_label) — pinned by hand
PINNED = {
    1: 6,   # z_v_grad_w indirect-stencil producer; existing loopnest_1
}

# Total loopnest folders to fill. The Fortran source has 7 distinct
# pattern classes (6 compute + 1 reduction). The reduction class
# (Nest 15 only) is dropped because it folds into the partial-vert
# direct-stencil class — picking the highest-impact 6 keeps the
# benchmark surface manageable.
N_LOOPNESTS = 6


@dataclass
class Nest:
    nid: int
    shape: str          # e.g. "v.h", "b.h", "v"
    ranges: list        # e.g. ["full_vert", "full_horiz"]
    behavior: str       # "compute" | "reduction" | "accumulate"
    collapsed: str      # e.g. "h:U  v:S  b:S"
    arrays: list        # unique array names
    fortran: str        # source snippet

    @property
    def n_arrays(self) -> int:
        return len(set(self.arrays))

    @property
    def collapsed_no_block(self) -> str:
        # strip "  b:S" suffix and any other "b:..." token
        return "  ".join(p for p in self.collapsed.split("  ") if not p.startswith("b:"))

    @property
    def shape_no_block(self) -> str:
        return ".".join(p for p in self.shape.split(".") if p != "b")

    @property
    def ranges_no_block(self) -> tuple:
        # block ranges live on jb axes only; keep h/v ranges
        return tuple(r for r in self.ranges if r not in ("full_block",))

    @property
    def pattern_key(self) -> tuple:
        return (self.shape_no_block, self.ranges_no_block,
                self.behavior, self.collapsed_no_block)

    @property
    def is_full_vert(self) -> bool:
        return "full_vert" in self.ranges_no_block


# ---------------------------------------------------------------------------
# parser
# ---------------------------------------------------------------------------

NEST_RE = re.compile(
    r"^### Nest (\d+): `([^`]+)` \(([^)]+)\)(?: `(\w+)`)?\s*$"
)
COLLAPSED_RE = re.compile(r"^Collapsed: `([^`]+)`\s*$")
ARRAY_ROW_RE = re.compile(r"^\| `([^`]+)` \| `[^`]*` \|\s*$")


def parse_analysis(path: Path) -> list[Nest]:
    nests: list[Nest] = []
    cur: Nest | None = None
    in_fortran = False
    in_array_table = False
    fortran_lines: list[str] = []

    for raw in path.read_text().splitlines():
        # Section break flushes the current nest so trailing array tables
        # in later sections don't bleed into it.
        if raw.startswith("## "):
            if cur is not None:
                cur.fortran = "\n".join(fortran_lines).strip()
                nests.append(cur)
                cur = None
                fortran_lines = []
            in_fortran = False
            in_array_table = False
            continue

        m = NEST_RE.match(raw)
        if m:
            if cur is not None:
                cur.fortran = "\n".join(fortran_lines).strip()
                nests.append(cur)
            nid = int(m.group(1))
            shape = m.group(2)
            ranges = [r.strip() for r in m.group(3).split(",")]
            behavior = m.group(4) or "compute"
            cur = Nest(nid=nid, shape=shape, ranges=ranges,
                       behavior=behavior, collapsed="", arrays=[], fortran="")
            in_fortran = False
            in_array_table = False
            fortran_lines = []
            continue

        if cur is None:
            continue

        cm = COLLAPSED_RE.match(raw)
        if cm:
            cur.collapsed = cm.group(1)
            continue

        if raw.startswith("```fortran"):
            in_fortran = True
            continue
        if in_fortran:
            if raw.startswith("```"):
                in_fortran = False
            else:
                fortran_lines.append(raw)
            continue

        # End-of-details closes any open table (section breaks handled above).
        if raw.startswith("</details>"):
            in_array_table = False
            continue

        if raw.startswith("|-------|"):
            in_array_table = True
            continue
        if in_array_table:
            if not raw.startswith("|"):
                in_array_table = False
                continue
            am = ARRAY_ROW_RE.match(raw)
            if am:
                cur.arrays.append(am.group(1))

    if cur is not None:
        cur.fortran = "\n".join(fortran_lines).strip()
        nests.append(cur)

    return nests


# ---------------------------------------------------------------------------
# selection
# ---------------------------------------------------------------------------

# Stable, human-friendly class labels per pattern_key (filled at runtime).
CLASS_DESCRIPTIONS = {
    ("v.h", ("full_vert", "full_horiz"), "compute", "h:U  v:S"):
        "indirect stencil, full vertical",
    ("v.h", ("partial_vert", "full_horiz"), "compute", "h:S  v:S"):
        "direct stencil, partial vertical",
    ("v.h", ("full_vert", "full_horiz"), "compute", "h:S  v:S"):
        "direct stencil, full vertical",
    ("v.h", ("partial_vert", "full_horiz"), "compute", "h:U  v:S"):
        "indirect stencil, partial vertical",
    ("h", ("full_horiz",), "compute", "h:S"):
        "horizontal-only (boundary)",
    ("v", ("partial_vert",), "compute", "v:S"):
        "vertical-only (level reduction)",
    ("v.h", ("partial_vert", "full_horiz"), "reduction", "h:S  v:S"):
        "direct stencil, reduction (CFL clip)",
}


def class_label(key) -> str:
    return CLASS_DESCRIPTIONS.get(key, "uncategorized")


def tiebreak_sort_key(n: Nest) -> tuple:
    # sort ascending → first element is the chosen one
    return (-n.n_arrays, 0 if n.is_full_vert else 1, n.nid)


def choose_per_class(nests: list[Nest]) -> dict:
    by_class: dict = defaultdict(list)
    for n in nests:
        by_class[n.pattern_key].append(n)

    # Stable class ordering: pinned classes first (lowest loopnest_id wins),
    # then largest cohort, then label. This guarantees pinned picks land in
    # their declared loopnest_id even if a larger cohort exists elsewhere.
    pinned_class_keys = set()
    for nest_id in PINNED.values():
        for n in nests:
            if n.nid == nest_id:
                pinned_class_keys.add(n.pattern_key)

    def order_key(k):
        is_pinned = 0 if k in pinned_class_keys else 1
        return (is_pinned, -len(by_class[k]), class_label(k))

    class_order = sorted(by_class.keys(), key=order_key)

    pinned_by_class: dict = {}
    for loopnest_id, nest_id in PINNED.items():
        for n in nests:
            if n.nid == nest_id:
                pinned_by_class[n.pattern_key] = (loopnest_id, n)
                break

    chosen = []  # list of dicts
    used_loopnest_ids = set(PINNED.keys())
    next_loopnest = 2

    for key in class_order:
        cohort = sorted(by_class[key], key=tiebreak_sort_key)
        if key in pinned_by_class:
            ln_id, pin_nest = pinned_by_class[key]
            chosen_nest = pin_nest
            reason = f"pinned to loopnest_{ln_id}"
        else:
            while next_loopnest in used_loopnest_ids:
                next_loopnest += 1
            ln_id = next_loopnest
            used_loopnest_ids.add(ln_id)
            chosen_nest = cohort[0]
            reason = "tiebreak: max-arrays, full_vert, lowest nid"

        chosen.append({
            "loopnest_id": ln_id,
            "class_key": list(key),
            "class_label": class_label(key),
            "cohort_size": len(cohort),
            "candidates": [
                {"nid": n.nid, "n_arrays": n.n_arrays,
                 "ranges": list(n.ranges_no_block)}
                for n in cohort
            ],
            "chosen_nid": chosen_nest.nid,
            "chosen_n_arrays": chosen_nest.n_arrays,
            "chosen_ranges": list(chosen_nest.ranges_no_block),
            "reason": reason,
        })

    chosen.sort(key=lambda c: c["loopnest_id"])

    # Cap at N_LOOPNESTS; surface the dropped classes for transparency.
    kept = [c for c in chosen if c["loopnest_id"] <= N_LOOPNESTS]
    dropped = [c for c in chosen if c["loopnest_id"] > N_LOOPNESTS]
    return {"selections": kept, "dropped": dropped}


# ---------------------------------------------------------------------------
# emitters
# ---------------------------------------------------------------------------

def emit_json(out: Path, nests: list[Nest], picks: dict):
    payload = {
        "source": "velocity_advection_preprocessed.analysis.md",
        "tiebreaker": [
            "widest (most unique arrays) descending",
            "full_vert before partial_vert",
            "lowest source-order nest id",
        ],
        "pinned": {f"loopnest_{k}": v for k, v in PINNED.items()},
        "all_nests": [
            {
                "nid": n.nid,
                "shape": n.shape_no_block,
                "ranges": list(n.ranges_no_block),
                "behavior": n.behavior,
                "collapsed": n.collapsed_no_block,
                "n_arrays": n.n_arrays,
                "arrays": sorted(set(n.arrays)),
                "pattern_key": list(n.pattern_key),
                "class_label": class_label(n.pattern_key),
            }
            for n in sorted(nests, key=lambda x: x.nid)
        ],
        **picks,
    }
    out.write_text(json.dumps(payload, indent=2))


def emit_markdown(out: Path, nests: list[Nest], picks: dict):
    L = []
    L.append("# Loopnest selection (deterministic)")
    L.append("")
    L.append("Generated by `select_loopnests.py`. Re-run after editing")
    L.append("`PINNED` or the analysis source.")
    L.append("")
    L.append("## Tiebreaker")
    L.append("")
    L.append("Within a pattern class, the chosen nest is picked by:")
    L.append("")
    L.append("1. most unique array names (widest)")
    L.append("2. `full_vert` before `partial_vert`")
    L.append("3. lowest source-order nest id")
    L.append("")
    L.append("Pinned overrides bypass the heuristic:")
    for ln_id, nid in PINNED.items():
        L.append(f"- `loopnest_{ln_id}` ↔ Nest {nid}")
    L.append("")
    L.append("## Chosen loopnests")
    L.append("")
    L.append("| Loopnest | Class | Chosen Nest | #arrays | Ranges | Cohort | Reason |")
    L.append("|---------:|-------|------------:|--------:|--------|-------:|--------|")
    for sel in picks["selections"]:
        ranges = ", ".join(sel["chosen_ranges"]) or "—"
        L.append(
            f"| {sel['loopnest_id']} | {sel['class_label']} "
            f"| {sel['chosen_nid']} | {sel['chosen_n_arrays']} "
            f"| {ranges} | {sel['cohort_size']} | {sel['reason']} |"
        )
    L.append("")
    if picks.get("dropped"):
        L.append(f"## Classes dropped (cap = {N_LOOPNESTS})")
        L.append("")
        L.append("| Class | Cohort | Best candidate | #arrays |")
        L.append("|-------|-------:|---------------:|--------:|")
        for sel in picks["dropped"]:
            L.append(
                f"| {sel['class_label']} | {sel['cohort_size']} "
                f"| Nest {sel['chosen_nid']} | {sel['chosen_n_arrays']} |"
            )
        L.append("")
    L.append("## All candidates per class")
    L.append("")
    by_id = {n.nid: n for n in nests}
    for sel in picks["selections"]:
        L.append(f"### loopnest_{sel['loopnest_id']} — {sel['class_label']}")
        L.append("")
        L.append(f"Pattern key: `{sel['class_key']}`")
        L.append("")
        L.append("| Nest | #arrays | Ranges | Chosen |")
        L.append("|-----:|--------:|--------|:------:|")
        for cand in sel["candidates"]:
            mark = "✅" if cand["nid"] == sel["chosen_nid"] else ""
            ranges = ", ".join(cand["ranges"]) or "—"
            L.append(f"| {cand['nid']} | {cand['n_arrays']} | {ranges} | {mark} |")
        L.append("")
        chosen = by_id[sel["chosen_nid"]]
        L.append("<details><summary>Chosen Fortran snippet</summary>")
        L.append("")
        L.append("```fortran")
        L.append(chosen.fortran)
        L.append("```")
        L.append("")
        L.append("Arrays: " + ", ".join(f"`{a}`" for a in sorted(set(chosen.arrays))))
        L.append("")
        L.append("</details>")
        L.append("")
    L.append("## Full nest inventory")
    L.append("")
    L.append("| Nest | Shape | Ranges | Behavior | Collapsed | #arrays | Class |")
    L.append("|-----:|-------|--------|----------|-----------|--------:|-------|")
    for n in sorted(nests, key=lambda x: x.nid):
        ranges = ", ".join(n.ranges_no_block) or "—"
        beh = "" if n.behavior == "compute" else f"`{n.behavior}`"
        L.append(
            f"| {n.nid} | `{n.shape_no_block}` | {ranges} | {beh} "
            f"| `{n.collapsed_no_block}` | {n.n_arrays} | {class_label(n.pattern_key)} |"
        )
    out.write_text("\n".join(L) + "\n")


# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--analysis", type=Path, default=DEFAULT_ANALYSIS)
    ap.add_argument("--out-json", type=Path,
                    default=HERE / "layout_candidates.json")
    ap.add_argument("--out-md", type=Path,
                    default=HERE / "chosen_loopnests.md")
    args = ap.parse_args()

    if not args.analysis.exists():
        raise SystemExit(f"missing analysis: {args.analysis}\n"
                         "Run `python analyze_access.py velocity_advection_preprocessed.f90` first.")

    nests = parse_analysis(args.analysis)
    if not nests:
        raise SystemExit("parser found 0 nests; analysis.md format may have changed")

    picks = choose_per_class(nests)

    emit_json(args.out_json, nests, picks)
    emit_markdown(args.out_md, nests, picks)

    # console summary
    print(f"parsed {len(nests)} nests from {args.analysis.name}")
    print(f"wrote   {args.out_json.name}")
    print(f"wrote   {args.out_md.name}")
    print()
    print("Chosen:")
    for sel in picks["selections"]:
        print(f"  loopnest_{sel['loopnest_id']:<2} ← Nest {sel['chosen_nid']:>2} "
              f"({sel['chosen_n_arrays']} arr) {sel['class_label']}")


if __name__ == "__main__":
    main()
