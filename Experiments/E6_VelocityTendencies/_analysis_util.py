"""
_analysis_util.py  — shared helpers for the E6 analysis scripts.

Consumed by:
  * analyze_blocked_vs_unblocked.py
  * build_layout_crossproduct.py

Everything below is CSV schema plumbing: locating the right CSV per
(loopnest, platform, backend), recognizing the two schemas that
loopnest_1 vs loopnest_{2..6} use, and pulling out a single
`layout_id` string per row so the rest of the pipeline can ignore the
schema split.
"""

import csv
import statistics
from collections import defaultdict
from pathlib import Path

EXP_DIR = Path(__file__).resolve().parent
LOOPNEST_IDS = (1, 2, 3, 4, 5, 6)
PLATFORMS = ("beverin", "daint")
ALL_BACKENDS = ("cpu", "gpu")
IDENTITY_LAYOUT_ID = "V1"


# ──────────────────────────────────────────────────────────────────────
#  CSV discovery
# ──────────────────────────────────────────────────────────────────────

def csv_for(loopnest_id, platform, backend):
    """Return the one per-kernel CSV under
    `loopnest_{N}/results/{platform}/`. Skips the `metrics_*` and
    `*_old.csv` files that aren't benchmark output."""
    d = EXP_DIR / f"loopnest_{loopnest_id}" / "results" / platform
    if not d.is_dir():
        return None
    for p in sorted(d.glob(f"*_{backend}.csv")):
        if p.name.startswith("metrics") or p.name.endswith("_old.csv"):
            continue
        return p
    return None


# ──────────────────────────────────────────────────────────────────────
#  layout_id extraction — normalizes two schemas to a single string
# ──────────────────────────────────────────────────────────────────────

def detect_schema(header, loopnest_id, backend):
    """Return one of: 'l1_cpu', 'l1_gpu', 'new_cpu', 'new_gpu'.

    loopnest_1 uses `variant` (+ `blocking` on CPU); loopnest_{2..6}
    use `V` (+ `layout` on CPU, `config_label` on GPU)."""
    if "variant" in header:
        return "l1_cpu" if backend == "cpu" else "l1_gpu"
    if "V" in header:
        return "new_cpu" if backend == "cpu" else "new_gpu"
    raise RuntimeError(
        f"loopnest_{loopnest_id} ({backend}): unknown CSV schema "
        f"{list(header)}"
    )


def _layout_id_l1_cpu(row):
    V, B = int(row["variant"]), int(row["blocking"])
    if V > 0 and B == 0:
        return f"V{V}"
    if V == 0 and B > 0:
        return f"B{B}"
    return f"V{V}_B{B}"


def _layout_id_l1_gpu(row):
    # On L1 GPU all V=0 rows share the blocked-row-major layout (the
    # config_label prefix `t{TX}x{TY}` varies WITHIN a layout); V>=1 are
    # unblocked permutations.
    return f"V{int(row['variant'])}"


def _layout_id_new_cpu(row):
    lay = row["layout"]
    return f"B{lay}" if lay.isdigit() else lay


def _layout_id_new_gpu(row):
    cfg = row["config_label"]
    if cfg.startswith("B"):
        return cfg.split("_", 1)[0]
    return f"V{int(row['V'])}"


def extract_layout_id(row, schema, backend):
    if schema == "l1_cpu":
        return _layout_id_l1_cpu(row)
    if schema == "l1_gpu":
        return _layout_id_l1_gpu(row)
    return _layout_id_new_cpu(row) if backend == "cpu" else _layout_id_new_gpu(row)


def is_blocked_layout_id(lid):
    """B{n} or t{TX}x{TY} — every blocked/tiled-storage family."""
    return lid.startswith("B") or lid.startswith("t")


# ──────────────────────────────────────────────────────────────────────
#  Storage-axis projection
#
#  A micro-bench V-id is a JOINT choice on two orthogonal storage axes:
#
#    IC axis (innermost of a 2D/3D (h,v,...) field):
#        V1,V2   -> h_first (je is stride-1)
#        V3..V7  -> v_first (jk is stride-1)
#
#    IN axis (layout of a (N,2) connectivity pair):
#        V{odd}  -> SoA  (each neighbor column contiguous)
#        V{even} -> AoS  ({nbr0,nbr1} pairs contiguous)
#
#  V5/V6/V7 map to the same (IC, IN) storage as V3/V4; they differ only
#  in the GPU kernel schedule, which DaCe ignores when the global sweep
#  applies a storage choice. The full-sweep builder uses these
#  projections so groups only see votes on the axis they actually have.
# ──────────────────────────────────────────────────────────────────────

IC_CHOICES = ("h_first", "v_first")
IN_CHOICES = ("SoA", "AoS")


def v_to_ic(lid):
    """V{n} -> 'h_first' / 'v_first'. Returns None for blocked / unknown."""
    if not lid.startswith("V") or not lid[1:].isdigit():
        return None
    n = int(lid[1:])
    return "h_first" if n <= 2 else "v_first"


def v_to_in(lid):
    """V{n} -> 'SoA' / 'AoS'. Returns None for blocked / unknown."""
    if not lid.startswith("V") or not lid[1:].isdigit():
        return None
    return "SoA" if int(lid[1:]) % 2 == 1 else "AoS"


def project_layout_to_axis(lid, axis):
    """Project a V-id onto one storage axis ('IC' or 'IN')."""
    if axis == "IC":
        return v_to_ic(lid)
    if axis == "IN":
        return v_to_in(lid)
    raise ValueError(f"unknown axis: {axis}")


# ──────────────────────────────────────────────────────────────────────
#  median loader
# ──────────────────────────────────────────────────────────────────────

def load_medians(csv_path, loopnest_id, backend, nlev, cell_dist):
    """Return `{layout_id: median(time_ms)}` filtered to (nlev, cell_dist).
    The median is taken across every (schedule, config_label, run_id)
    sample for that layout_id."""
    rows = list(csv.DictReader(open(csv_path)))
    if not rows:
        return {}
    schema = detect_schema(rows[0].keys(), loopnest_id, backend)
    nlev_str = str(nlev)
    bucket = defaultdict(list)
    for r in rows:
        if r.get("nlev") != nlev_str:
            continue
        if "cell_dist" in r and cell_dist and r["cell_dist"] != cell_dist:
            continue
        try:
            lid = extract_layout_id(r, schema, backend)
            t = float(r["time_ms"])
        except (KeyError, ValueError, TypeError):
            continue
        bucket[lid].append(t)
    return {lid: statistics.median(ts) for lid, ts in bucket.items() if ts}
