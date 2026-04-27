#!/usr/bin/env python3
"""Derive per-loopnest V_k winners from E6 CSVs and emit ``winners.json``.

For every (loopnest, platform, backend) triple, walk the per-rep CSV
under ``Experiments/E6_VelocityTendencies/loopnest_<N>/results/<plat>/``,
find the V_k (variant index) that achieves the lowest median runtime
under realistic R02B05 connectivity (``cell_dist=exact``), and write
the result to ``access_analysis/winners.json``.

Reviewers can audit the E6→E7 chain by reading this JSON: it lists the
empirical V_k winner per representative loop nest, alongside the
median time and the runner-up (so you can see whether the win was
decisive). E7's ``permute_configs.py`` then composes the per-class
V_k assignments declared in ``layout_groups.json`` with these
empirical winners; the named ``winner_v1`` / ``winner_v2`` /
``winner_v6`` configs correspond to globally-uniform V_k applications.

Run:
    python derive_winners.py

Input:  Experiments/E6_VelocityTendencies/loopnest_<1..6>/results/<plat>/<kernel>_<dev>.csv
Output: access_analysis/winners.json (next to this script)

V_k naming convention (E6's ``_analysis_util.py``):
    V1, V2 = h_first (je stride-1);  V_odd = SoA, V_even = AoS
    V3..V7 = v_first (jk stride-1);  V_odd = SoA, V_even = AoS
    V0     = blocked layout (out of scope for E7's V_k composition)
"""
from __future__ import annotations

import csv
import json
import statistics
from collections import defaultdict
from pathlib import Path

HERE        = Path(__file__).resolve().parent
EXP_DIR     = HERE.parent
LOOPNESTS   = (1, 2, 3, 4, 5, 6)
PLATFORMS   = ("daint", "beverin")
BACKENDS    = ("cpu", "gpu")
CELL_DIST   = "exact"            # R02B05 realistic connectivity
OUT_PATH    = HERE / "winners.json"


def _csv_for(loopnest: int, platform: str, backend: str) -> Path | None:
    """Locate the per-rep CSV for one (loopnest, platform, backend).
    Returns ``None`` if no matching CSV exists. The naming is
    ``<kernel>_<backend>.csv`` and there is at most one per dir; we
    pick the first non-``metrics_`` / non-``_old`` match the same way
    ``_analysis_util.py:csv_for`` does."""
    d = EXP_DIR / f"loopnest_{loopnest}" / "results" / platform
    if not d.is_dir():
        return None
    for p in sorted(d.glob(f"*_{backend}.csv")):
        if p.name.startswith("metrics") or p.name.endswith("_old.csv"):
            continue
        return p
    return None


def _vk_column(header: list[str]) -> str | None:
    """L1 used ``variant``; L2..L6 use ``V``. Either way, returns the
    column that holds the V-id integer."""
    if "V" in header:
        return "V"
    if "variant" in header:
        return "variant"
    return None


def _median_time_per_vk(csv_path: Path) -> dict[int, float]:
    """Return ``{V_k: median(time_ms)}`` over rows that match
    ``CELL_DIST``. V_k=0 (blocked layout) is excluded; only the
    unblocked V1..V_n are returned because the AD's E7 composition
    operates on storage permutations, not on blocking."""
    by_vk: dict[int, list[float]] = defaultdict(list)
    with csv_path.open() as f:
        reader = csv.DictReader(f)
        if reader.fieldnames is None:
            return {}
        col = _vk_column(reader.fieldnames)
        if col is None:
            return {}
        for row in reader:
            if row.get("cell_dist") != CELL_DIST:
                continue
            try:
                vk = int(row[col])
                t = float(row["time_ms"])
            except (KeyError, TypeError, ValueError):
                continue
            if vk == 0:
                continue  # blocked layout, out of scope
            by_vk[vk].append(t)
    return {vk: statistics.median(ts) for vk, ts in by_vk.items() if ts}


def _winner_for(loopnest: int, platform: str, backend: str) -> dict | None:
    csv_path = _csv_for(loopnest, platform, backend)
    if csv_path is None:
        return None
    medians = _median_time_per_vk(csv_path)
    if not medians:
        return None
    ranked = sorted(medians.items(), key=lambda kv: kv[1])
    win_vk, win_t = ranked[0]
    runner = ranked[1] if len(ranked) > 1 else (None, None)
    return {
        "v_winner": f"V{win_vk}",
        "time_ms_median": round(win_t, 4),
        "runner_up": (f"V{runner[0]}" if runner[0] is not None else None),
        "runner_up_time_ms_median": (round(runner[1], 4)
                                     if runner[1] is not None else None),
        "ranked": [{"v": f"V{vk}", "time_ms_median": round(t, 4)}
                   for vk, t in ranked],
        "csv": str(csv_path.relative_to(EXP_DIR.parent.parent)),
    }


def _vk_family(vk_label: str) -> str:
    """Map a V_k label to its storage-axis category.
    ``_analysis_util.py``: V1/V2 = h_first, V3..V7 = v_first."""
    n = int(vk_label[1:])
    return "h_first" if n in (1, 2) else "v_first"


def _per_class_cross_product(winners: list[dict]) -> dict:
    """Build the cross-product axis of winners per (platform, backend).

    For each (platform, backend) pair we collect, per loopnest, the
    measured winning V_k. The cross product across all 6 loopnests
    enumerates every per-class V_k assignment a reviewer would have to
    consider when composing the E7 layout: for any loopnest you can
    either take its empirical winner or fall back to V1 (the identity
    baseline).

    Output schema::

        {
          "<platform>_<backend>": {
            "axes": {
              "loopnest_1": ["V1", "<winner>"],
              ...
              "loopnest_6": ["V1", "<winner>"]
            },
            "n_combinations": 64           # 2^6 (one binary choice per loopnest)
          },
          ...
        }

    E7's ``permute_configs.py`` can then enumerate these combinations
    and emit one ``cross_<plat>_<bk>_<bitmask>`` config per cell --
    the cross product whose two endpoints are ``winner_v1`` (all V1)
    and ``winner_per_class`` (every loopnest at its empirical winner).
    """
    by_target: dict[str, dict[int, str]] = defaultdict(dict)
    for w in winners:
        by_target[f"{w['platform']}_{w['backend']}"][w["loopnest"]] = w["v_winner"]
    out: dict[str, dict] = {}
    for target, per_ln in by_target.items():
        axes = {f"loopnest_{ln}": ["V1", per_ln[ln]]
                for ln in sorted(per_ln) if per_ln[ln] != "V1"}
        n = 1
        for opts in axes.values():
            n *= len(opts)
        out[target] = {"axes": axes, "n_combinations": n}
    return out


def main() -> None:
    payload: dict = {
        "schema_version": 1,
        "doc": "Empirical V_k winners derived from "
               "Experiments/E6_VelocityTendencies/loopnest_<N>/results/. "
               "Read by E7 (and the AD reader) to verify that the "
               "named winner_v* configs in E7 match the per-loopnest "
               "V_k that E6 measured under R02B05 connectivity.",
        "cell_dist": CELL_DIST,
        "winners": [],
        "summary": {},
        "cross_product": {},
    }
    family_votes: dict[str, dict[str, int]] = defaultdict(lambda: defaultdict(int))
    for ln in LOOPNESTS:
        for plat in PLATFORMS:
            for bk in BACKENDS:
                w = _winner_for(ln, plat, bk)
                if w is None:
                    continue
                row = {"loopnest": ln, "platform": plat, "backend": bk, **w}
                payload["winners"].append(row)
                family_votes[f"{plat}_{bk}"][_vk_family(w["v_winner"])] += 1

    # Per-target tally so the AD can quote "v_first wins on N of M
    # loopnests on platform X" without re-aggregating downstream.
    payload["summary"] = {
        target: dict(votes) for target, votes in family_votes.items()
    }
    # Per-target cross product: every binary toggle per loopnest. This
    # is the auditable spec the E7 sweep is supposed to cover.
    payload["cross_product"] = _per_class_cross_product(payload["winners"])

    OUT_PATH.write_text(json.dumps(payload, indent=2) + "\n")
    print(f"wrote {OUT_PATH}")
    print(f"  winners across {len(payload['winners'])} (loopnest, platform, backend) triples")
    for target, votes in sorted(payload["summary"].items()):
        n_v = votes.get("v_first", 0)
        n_h = votes.get("h_first", 0)
        n_cells = payload["cross_product"][target]["n_combinations"]
        print(f"  {target}: v_first={n_v}  h_first={n_h}  cross_product_cells={n_cells}")


if __name__ == "__main__":
    main()
