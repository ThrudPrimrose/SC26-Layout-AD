"""
v123_bridge.py -- register V_k cross-product configs into E8's
PERMUTE_CONFIGS, mirroring E7's tools/run_layout_configs.py logic.

Source-of-truth files:
- ``E6/access_analysis/canonical_array_groups.json`` (6 groups + per-group
  arrays + per-group axis: IC for data, IN for connectivity)
- ``E6/full_velocity_tendencies/layout_crossproduct_winners.json``
  (already-enumerated V_k cross-product cells with pre-computed axis
  projections, e.g. ``cv_IC: h_first|v_first``, ``n_IN: SoA|AoS``)

Per-cell registration: build a ``{e8_array_name: permutation}`` dict by
looking up each group's arrays in E8's known PermMap (the union of
_COMPUTE_VERT_PERMUTED, _COMPUTE_HORIZ_PERMUTED, _FIELDS_PERMUTED,
_STENCIL_PERMUTED, and _CONN_ARRAYS). Arrays that don't appear in
E8's known set (e.g. local connectivity aliases like ``icblk`` that
the SDFG doesn't expose) are silently skipped -- those slots are
already covered by E8's existing connectivity strategy via
``_conn_map()`` applied to ``_CONN_ARRAYS``.

Pure stdlib so this module loads even when DaCe is unavailable
(needed for ``run_stage8_permutations.py --dry-run``).
"""

from __future__ import annotations

import json
from itertools import product
from pathlib import Path
from typing import Callable, Dict, List, Tuple

_E6_DIR = Path(__file__).resolve().parents[2] / "E6_VelocityTendencies"
DEFAULT_GROUPS_JSON = _E6_DIR / "access_analysis" / "canonical_array_groups.json"
DEFAULT_WINNERS_JSON = _E6_DIR / "full_velocity_tendencies" / "layout_crossproduct_winners.json"


def fortran_to_sdfg_array_name(s: str) -> str:
    """Mirror E7's translation: ``p_diag%foo`` -> ``__CG_p_diag__m_foo``."""
    if "%" not in s:
        return s
    return "__CG_" + s.replace("%", "__m_").strip()


def _gpu(s: str) -> str:
    """E8 stage-8 SDFGs use a ``gpu_`` prefix on every array."""
    return f"gpu_{s}"


# Local Fortran aliases for connectivity arrays (the ``n`` group in
# ``canonical_array_groups.json``) -> long DaCe SDFG names. Hand-coded
# because LOKI's ``INDIRECT_ARRAYS`` filter strips the long names from
# per-nest array lists, so the access-analysis JSON only carries the
# aliases. Without this resolver, every v123 cell collapses on the IN
# axis (every ``n`` array maps to nothing in the SDFG), so V_n choices
# become invisible. Each alias expands to ONE long name -- if you add a
# new alias, also extend this map.
_CONN_ALIASES: Dict[str, str] = {
    # cell -> edge connectivity (cell index lives on edges)
    "icblk":  "gpu___CG_p_patch__CG_edges__m_cell_blk",
    "icidx":  "gpu___CG_p_patch__CG_edges__m_cell_idx",
    # edge -> cell (edge index lives on cells)
    "ieblk":  "gpu___CG_p_patch__CG_cells__m_edge_blk",
    "ieidx":  "gpu___CG_p_patch__CG_cells__m_edge_idx",
    # cell -> neighbor cell
    "incblk": "gpu___CG_p_patch__CG_cells__m_neighbor_blk",
    "incidx": "gpu___CG_p_patch__CG_cells__m_neighbor_idx",
    # quad-edge: edge -> 4 surrounding edges
    "iqblk":  "gpu___CG_p_patch__CG_edges__m_quad_blk",
    "iqidx":  "gpu___CG_p_patch__CG_edges__m_quad_idx",
    # vertex on edge endpoints
    "ivblk":  "gpu___CG_p_patch__CG_edges__m_vertex_blk",
    "ividx":  "gpu___CG_p_patch__CG_edges__m_vertex_idx",
}


_NLEV_FIRST_BY_DIM = {2: [1, 0], 3: [1, 0, 2], 4: [1, 0, 2, 3]}
_AOS_3D = [0, 2, 1]


def build_permute_map_for_cell(
    cell: Dict,
    group_arrays: Dict[str, List[str]],
    group_axis: Dict[str, str],
    sdfg_array_dims: Dict[str, int],
) -> Dict[str, List[int]]:
    """Translate one v123-JSON cell into a {e8_array_name: permutation} map.

    Reads pre-computed projections (``<gid>_<axis>`` -- e.g. ``cv_IC``)
    from the cell. Arrays not in *sdfg_array_dims* are skipped silently.
    """
    pmap: Dict[str, List[int]] = {}
    for gid, arrs in group_arrays.items():
        axis = group_axis.get(gid, "IC")
        proj = cell.get(f"{gid}_{axis}")
        for fortran_name in arrs:
            # Resolve connectivity-group local aliases (icblk, icidx, ...)
            # to their long DaCe names. For all other groups,
            # ``_CONN_ALIASES.get`` returns None and we fall through to
            # the generic Fortran -> SDFG translation.
            sdfg_name = _CONN_ALIASES.get(fortran_name)
            if sdfg_name is None:
                sdfg_name = _gpu(fortran_to_sdfg_array_name(fortran_name))
            ndim = sdfg_array_dims.get(sdfg_name)
            if ndim is None:
                continue
            if axis == "IC" and proj == "v_first":
                perm = _NLEV_FIRST_BY_DIM.get(ndim)
                if perm is not None:
                    pmap[sdfg_name] = list(perm)
            elif axis == "IN" and proj == "AoS" and ndim == 3:
                pmap[sdfg_name] = list(_AOS_3D)
            # Otherwise (h_first / SoA / unknown axis): identity, omit from map.
    return pmap


def cell_label(cell: Dict, group_ids: List[str]) -> str:
    """``v123_cv_V1_ch_V1_f_V6_s_V6_n_V6_lm_V6`` etc., matches E7's format."""
    parts = [f"{gid}_{cell[gid]}" for gid in group_ids]
    return "v123_" + "_".join(parts)


def _signature(pmap: Dict[str, List[int]]) -> str:
    return json.dumps(pmap, sort_keys=True)


def register_v123_into(
    permute_configs: Dict,
    make_config: Callable[[Dict[str, List[int]]], Dict],
    sdfg_array_dims: Dict[str, int],
    *,
    groups_json: Path = DEFAULT_GROUPS_JSON,
    winners_json: Path = DEFAULT_WINNERS_JSON,
) -> Tuple[List[str], int]:
    """Read groups + winners JSON, register every unique v123 cell into
    *permute_configs* via *make_config*.

    Returns (registered_names, raw_cell_count). Names already present in
    *permute_configs* are not overwritten. Duplicate permute-map
    signatures across cells collapse to the first cell's name.
    """
    if not groups_json.exists() or not winners_json.exists():
        return [], 0

    groups = json.loads(groups_json.read_text())
    cross = json.loads(winners_json.read_text())

    group_ids: List[str] = groups["group_ids"]
    group_arrays: Dict[str, List[str]] = groups["arrays"]
    group_axis: Dict[str, str] = groups["group_axis"]
    cells = cross.get("crossproduct", [])

    seen_sigs: Dict[str, str] = {}
    registered: List[str] = []
    for cell in cells:
        pmap = build_permute_map_for_cell(cell, group_arrays, group_axis, sdfg_array_dims)
        sig = _signature(pmap)
        if sig in seen_sigs:
            continue
        seen_sigs[sig] = cell_label(cell, group_ids)
        name = seen_sigs[sig]
        if name in permute_configs:
            continue
        permute_configs[name] = make_config(pmap)
        registered.append(name)
    return registered, len(cells)
