"""Hand-tuned ``PermuteConfig`` set for the velocity-tendencies sweep.

The heuristic-derived configs in :mod:`utils.passes.permute_configs`
(``unpermuted``, ``nlev_first``, ``index_only``, ``nest_<nid>_<shape>``)
are mechanically generated from the E6 access-analysis JSON. They cover
the easy cases but miss researcher-validated specifics -- for instance,
the heuristic emits ``[2, 0, 1]`` for connectivity arrays whereas
empirical sweeps showed ``[0, 2, 1]`` is the actually-correct
permutation for the velocity kernels.

This module is the cleaned-up port of the icon-artifacts research
scratch (``velocity/sc26_layout/permute_stage4.py``). Only the
**transformation data** survived the port: the four per-group array →
permutation maps, the 14 connectivity arrays, the 95-cell sweep
generator, and the two named configs ``curated_nlev_first`` /
``curated_index_only``. Everything else from that file
(``permute_sdfg``, ``add_timers``, ``update_reductions``,
``numa_touch_for_config``, kernel-extraction drivers, plotting, sample
drivers, microbenchmarks) was either already cleanly re-implemented
elsewhere in the pipeline (timers + async reductions live in stage 4;
the canonical permutation pass lives in DaCe at
``dace.transformation.layout.permute_dimensions``) or was research
scratch that does not belong in the AD freeze.

API:

    curated_configs(sdfg_arrays=None) -> List[PermuteConfig]
        96 entries: 94 sweep cells (``cv<0|1>_ch<0|1>_f<0|1>_s<0|1>_n<perm>``
        excluding the all-zero/identity-conn cell which equals
        ``unpermuted``) plus ``curated_nlev_first`` and
        ``curated_index_only``.

The names are runnable directly via
``python -m utils.stages.stage6 --config <name>`` once
``extended_configs_from_candidates`` includes the curated set.
"""

from itertools import permutations as _iter_perms
from typing import Dict, List

from .permute_layout import PermuteConfig

Perm = List[int]
PermMap = Dict[str, Perm]


# ---------------------------------------------------------------------------
# Per-group array → permutation maps (researcher-tuned).
#
# All keys are post-stage-4 ``gpu_<name>`` GPU mirrors. ``OffloadVelocityToGPU``
# both creates explicit ``gpu_<host>`` mirrors for kernel-touched non-
# transients AND renames pure-scratch transients in place to ``gpu_<orig>``,
# so every kernel-side array carries the prefix. Keying on ``gpu_X`` is
# what lets the per-transient logic in PermuteDimensions emit GPU<->GPU
# transpose kernels and keep the boundary cudaMemcpy on the unpermuted
# layout. The ranks here match the post-stage-4 SDFG (e.g. ``gpu_z_w_con_c``
# is rank-3 after the AoS->SoA expansion, not the rank-2 form in the
# Fortran-level analysis).
#
# Sweep architecture (matches paper §IV-A, "5 distinct loop patterns ...
# 7 layout-equivalent groups"):
#
# Naming convention: V1 ... V6 enumerate the six rank-3 permutations
# (V1 = [0,1,2] identity, V2 = [1,0,2] level-first, V5 = [2,1,0],
# V6 = [0,2,1] index-axis-second, etc.). E6 access analysis selects
# the best V_k per group; for most groups V2 is the recommended
# alternative to V1, but for connectivity-style indirect-index access
# V6 is the validated choice (and V2 / V3 were rejected -- see comments
# at the connectivity entries below). For untouched groups (those E6
# doesn't classify) we sweep every V_k of the appropriate rank.
#
#   E6-classified groups -- binary axis (V1 vs the E6-recommended V_k):
#     cv  -- rank-3 vertical-accumulation work arrays   (V1 vs V2)
#     ch  -- rank-3 horizontal-stencil work arrays      (V1 vs V2)
#     f   -- rank-3 prog/diag + rank-4 ddt fields       (V1 vs V2 = [1,0,2,3])
#     s   -- rank-3 read-only metrics + interpolation   (V1 vs V2)
#     lv  -- rank-2 ``gpu_levmask``                     (V1 vs [1,0])
#
#   Untouched groups -- full permutation sweep:
#     n   -- rank-3 connectivity tables (6 permutations; E6 also
#            recommends V6 = [0,2,1], but we sweep all six to confirm)
#     em  -- rank-2 edge / cell scalar metrics (2 permutations = binary;
#            kept on its own axis so the sweep covers it independently
#            of the s axis)
#
# Total sweep cells: 2^5 (E6 binary) * 6 (n) * 2 (em) * 2 (sm map shuffle)
# minus the all-zero identity duplicate of ``unpermuted``.
# Blocking is omitted globally -- empirical sweep showed no gain (paper
# §IV-A, "blocking candidates are pruned analogously").
# ---------------------------------------------------------------------------

# cv -- COMPUTE_VERT: vertical-accumulation arrays. jk is the natural
# parallelism axis. Level-first [1, 0, 2] makes jk-access stride-1.
_COMPUTE_VERT_PERMUTED: PermMap = {
    "gpu_z_w_con_c_full":   [1, 0, 2],
    "gpu_z_w_concorr_me":   [1, 0, 2],
    "gpu_z_w_v":            [1, 0, 2],
    "gpu_z_w_concorr_mc":   [1, 0, 2],
    "gpu_z_w_con_c":        [1, 0, 2],
}

# ch -- COMPUTE_HORIZ: horizontal-stencil arrays. je/jc is the
# innermost loop. Level-first breaks stride-1; included in the sweep
# as a hypothesis to test.
_COMPUTE_HORIZ_PERMUTED: PermMap = {
    "gpu_z_ekinh":          [1, 0, 2],
    "gpu_z_kin_hor_e":      [1, 0, 2],
    "gpu_z_vt_ie":          [1, 0, 2],
    "gpu_z_v_grad_w":       [1, 0, 2],
    "gpu_zeta":             [1, 0, 2],
    "gpu_maxvcfl":          [1, 0, 2],
    "gpu_cfl_clipping":     [1, 0, 2],
}

# lv -- LEVMASK group: ``gpu_levmask`` (post-stage-4 rank-2 GPU mirror).
# The host ``levmask`` is gone after stage 4 -- the array is renamed in
# place by ``OffloadVelocityToGPU``. Lives on its own axis so every
# config in the sweep is doubled into an ``_lv0`` (untouched) and ``_lv1``
# (transposed to ``[1, 0]``) pair.
_LEVMASK_PERMUTED: PermMap = {
    "gpu_levmask":          [1, 0],
}

# f -- FIELDS: prognostic + diagnostic arrays read in both vertical and
# horizontal loops. 4-D arrays have ntl as the last dim, so [1, 0, 2, 3].
_FIELDS_PERMUTED: PermMap = {
    "gpu___CG_p_prog__m_vn":              [1, 0, 2],
    "gpu___CG_p_prog__m_w":               [1, 0, 2],
    "gpu___CG_p_diag__m_vt":              [1, 0, 2],
    "gpu___CG_p_diag__m_vn_ie":           [1, 0, 2],
    "gpu___CG_p_diag__m_vn_ie_ubc":       [1, 0, 2],
    "gpu___CG_p_diag__m_w_concorr_c":     [1, 0, 2],
    "gpu___CG_p_diag__m_ddt_vn_apc_pc":   [1, 0, 2, 3],
    "gpu___CG_p_diag__m_ddt_w_adv_pc":    [1, 0, 2, 3],
    "gpu___CG_p_diag__m_ddt_vn_cor_pc":   [1, 0, 2, 3],
}

# s -- STENCIL: rank-3 read-only metric + interpolation coefficients.
# E6-classified (touched in v.h indirect-stencil nests), binary axis.
_STENCIL_PERMUTED: PermMap = {
    # Metrics rank 3
    "gpu___CG_p_metrics__m_wgtfac_e":          [1, 0, 2],
    "gpu___CG_p_metrics__m_wgtfacq_e":         [1, 0, 2],
    "gpu___CG_p_metrics__m_wgtfac_c":          [1, 0, 2],
    "gpu___CG_p_metrics__m_ddxn_z_full":       [1, 0, 2],
    "gpu___CG_p_metrics__m_ddxt_z_full":       [1, 0, 2],
    "gpu___CG_p_metrics__m_ddqz_z_half":       [1, 0, 2],
    "gpu___CG_p_metrics__m_ddqz_z_full_e":     [1, 0, 2],
    "gpu___CG_p_metrics__m_coeff1_dwdz":       [1, 0, 2],
    "gpu___CG_p_metrics__m_coeff2_dwdz":       [1, 0, 2],
    "gpu___CG_p_metrics__m_coeff_gradekin":    [1, 0, 2],
    # Interpolation rank 3
    "gpu___CG_p_int__m_e_bln_c_s":             [1, 0, 2],
    "gpu___CG_p_int__m_c_lin_e":               [1, 0, 2],
    "gpu___CG_p_int__m_geofac_n2s":            [1, 0, 2],
    "gpu___CG_p_int__m_geofac_grdiv":          [1, 0, 2],
    "gpu___CG_p_int__m_cells_aw_verts":        [1, 0, 2],
    "gpu___CG_p_int__m_geofac_rot":            [1, 0, 2],
    "gpu___CG_p_int__m_rbf_vec_coeff_e":       [1, 0, 2],
}

# em -- EDGE/CELL scalar metric tables (rank 2). Untouched by E6 (these
# don't appear as primary loop variables in any of the analyzed loop
# nests), so per the paper architecture we sweep all permutations of
# rank 2 (= 2 permutations = binary axis). Kept separate from `s` so
# the sweep can permute them independently of the rank-3 stencils.
_EDGE_METRIC_PERMUTED: PermMap = {
    "gpu___CG_p_patch__CG_edges__m_area_edge":              [1, 0],
    "gpu___CG_p_patch__CG_edges__m_f_e":                    [1, 0],
    "gpu___CG_p_patch__CG_edges__m_inv_primal_edge_length": [1, 0],
    "gpu___CG_p_patch__CG_edges__m_inv_dual_edge_length":   [1, 0],
    "gpu___CG_p_patch__CG_edges__m_tangent_orientation":    [1, 0],
    "gpu___CG_p_patch__CG_cells__m_area":                   [1, 0],
}

# n -- CONN: connectivity index arrays in (nproma, nblks, N) layout.
_CONN_ARRAYS: List[str] = [
    "gpu___CG_p_patch__CG_cells__m_edge_idx",
    "gpu___CG_p_patch__CG_cells__m_edge_blk",
    "gpu___CG_p_patch__CG_cells__m_neighbor_idx",
    "gpu___CG_p_patch__CG_cells__m_neighbor_blk",
    "gpu___CG_p_patch__CG_edges__m_cell_idx",
    "gpu___CG_p_patch__CG_edges__m_cell_blk",
    "gpu___CG_p_patch__CG_edges__m_vertex_idx",
    "gpu___CG_p_patch__CG_edges__m_vertex_blk",
    "gpu___CG_p_patch__CG_edges__m_quad_idx",
    "gpu___CG_p_patch__CG_edges__m_quad_blk",
    "gpu___CG_p_patch__CG_verts__m_cell_blk",
    "gpu___CG_p_patch__CG_verts__m_cell_idx",
    "gpu___CG_p_patch__CG_verts__m_edge_idx",
    "gpu___CG_p_patch__CG_verts__m_edge_blk",
]

# Explicitly out of scope: rank-1 arrays (``gpu_levelmask``, ``gpu_vcflmax``)
# and partition metadata (``gpu___CG_p_patch__CG_cells__CG_decomp_info__m_owner_mask``).
# These don't carry a layout to permute.

_ID_CONN: Perm = [0, 1, 2]
_ALL_CONN_PERMS: List[Perm] = [list(p) for p in _iter_perms([0, 1, 2])]


def _label(p: Perm) -> str:
    return "".join(str(i) for i in p)


def _conn_map(perm: Perm) -> PermMap:
    return {a: perm for a in _CONN_ARRAYS}


def _merge(*maps: PermMap) -> PermMap:
    out: PermMap = {}
    for m in maps:
        out.update(m)
    return out


def _filter_by_dim(pm: PermMap, sdfg_arrays: Dict[str, int] | None) -> PermMap:
    """Drop entries whose target SDFG array isn't present at the
    expected dimensionality. Equivalent to the same-named helper in
    :mod:`utils.passes.permute_configs`."""
    if sdfg_arrays is None:
        return pm
    out: PermMap = {}
    for name, perm in pm.items():
        ndim = sdfg_arrays.get(name)
        if ndim is None:
            continue
        if ndim != len(perm):
            continue
        out[name] = perm
    return out


def _cfg_name(cv: int, ch: int, f: int, s: int, np: Perm,
              em: int, lv: int, sm: int) -> str:
    return f"cv{cv}_ch{ch}_f{f}_s{s}_n{_label(np)}_em{em}_lv{lv}_sm{sm}"


def _build_sweep_map(cv: int, ch: int, f: int, s: int, np: Perm,
                     em: int, lv: int) -> PermMap:
    parts: List[PermMap] = []
    if cv:
        parts.append(_COMPUTE_VERT_PERMUTED)
    if ch:
        parts.append(_COMPUTE_HORIZ_PERMUTED)
    if f:
        parts.append(_FIELDS_PERMUTED)
    if s:
        parts.append(_STENCIL_PERMUTED)
    if np != _ID_CONN:
        parts.append(_conn_map(np))
    if em:
        parts.append(_EDGE_METRIC_PERMUTED)
    if lv:
        parts.append(_LEVMASK_PERMUTED)
    return _merge(*parts)


# ---------------------------------------------------------------------------
# Researcher-validated named configs (override the heuristic
# nlev_first / index_only with empirically-correct permutations).
# ---------------------------------------------------------------------------

# Connectivity uses [0, 2, 1] (NOT the heuristic [2, 0, 1]):
# (nproma, nblks, N) -> (nproma, N, nblks) keeps jc stride-1 after the
# gather on N. Past sweeps rejected [2, 1, 0] and [2, 0, 1].
_CURATED_NLEV_FIRST: PermMap = {
    # Compute-horiz / vert work arrays
    "gpu_z_ekinh":                                  [1, 0, 2],
    "gpu_z_kin_hor_e":                              [1, 0, 2],
    "gpu_z_v_grad_w":                               [1, 0, 2],
    "gpu_z_w_v":                                    [1, 0, 2],
    "gpu_zeta":                                     [1, 0, 2],
    "gpu_z_vt_ie":                                  [1, 0, 2],
    "gpu_z_w_concorr_me":                           [1, 0, 2],
    "gpu_z_w_concorr_mc":                           [1, 0, 2],
    "gpu_z_w_con_c_full":                           [1, 0, 2],
    "gpu_z_w_con_c":                                [1, 0, 2],
    "gpu_maxvcfl":                                  [1, 0, 2],
    "gpu_cfl_clipping":                             [1, 0, 2],
    # ``gpu_levmask`` lives on its own ``lv`` axis -- see _LEVMASK_PERMUTED
    # / ``curated_nlev_first_lv1`` below.
    # Prognostic / diagnostic
    "gpu___CG_p_prog__m_vn":                        [1, 0, 2],
    "gpu___CG_p_prog__m_w":                         [1, 0, 2],
    "gpu___CG_p_diag__m_vt":                        [1, 0, 2],
    "gpu___CG_p_diag__m_vn_ie":                     [1, 0, 2],
    "gpu___CG_p_diag__m_vn_ie_ubc":                 [1, 0, 2],
    "gpu___CG_p_diag__m_w_concorr_c":               [1, 0, 2],
    "gpu___CG_p_diag__m_ddt_vn_apc_pc":             [1, 0, 2, 3],
    "gpu___CG_p_diag__m_ddt_w_adv_pc":              [1, 0, 2, 3],
    "gpu___CG_p_diag__m_ddt_vn_cor_pc":             [1, 0, 2, 3],
    # Metrics
    "gpu___CG_p_metrics__m_wgtfac_e":               [1, 0, 2],
    "gpu___CG_p_metrics__m_wgtfacq_e":              [1, 0, 2],
    "gpu___CG_p_metrics__m_wgtfac_c":               [1, 0, 2],
    "gpu___CG_p_metrics__m_ddxn_z_full":            [1, 0, 2],
    "gpu___CG_p_metrics__m_ddxt_z_full":            [1, 0, 2],
    "gpu___CG_p_metrics__m_ddqz_z_half":            [1, 0, 2],
    "gpu___CG_p_metrics__m_ddqz_z_full_e":          [1, 0, 2],
    "gpu___CG_p_metrics__m_coeff1_dwdz":            [1, 0, 2],
    "gpu___CG_p_metrics__m_coeff2_dwdz":            [1, 0, 2],
    "gpu___CG_p_metrics__m_coeff_gradekin":         [1, 0, 2],
    # Interpolation -- identity (no-op); kept here for explicit listing
    "gpu___CG_p_int__m_e_bln_c_s":                  [0, 1, 2],
    "gpu___CG_p_int__m_c_lin_e":                    [0, 1, 2],
    "gpu___CG_p_int__m_geofac_n2s":                 [0, 1, 2],
    "gpu___CG_p_int__m_geofac_grdiv":               [0, 1, 2],
    "gpu___CG_p_int__m_cells_aw_verts":             [0, 1, 2],
    "gpu___CG_p_int__m_geofac_rot":                 [0, 1, 2],
    "gpu___CG_p_int__m_rbf_vec_coeff_e":             [0, 1, 2],
    # Edge metric rank 2 -- transpose to (nblks, nproma)
    "gpu___CG_p_patch__CG_edges__m_area_edge":              [1, 0],
    "gpu___CG_p_patch__CG_edges__m_f_e":                    [1, 0],
    "gpu___CG_p_patch__CG_edges__m_inv_primal_edge_length": [1, 0],
    "gpu___CG_p_patch__CG_edges__m_inv_dual_edge_length":   [1, 0],
    "gpu___CG_p_patch__CG_edges__m_tangent_orientation":    [1, 0],
    "gpu___CG_p_patch__CG_cells__m_area":                   [1, 0],
    # Connectivity (validated [0, 2, 1])
    "gpu___CG_p_patch__CG_verts__m_cell_blk":       [0, 2, 1],
    "gpu___CG_p_patch__CG_verts__m_cell_idx":       [0, 2, 1],
    "gpu___CG_p_patch__CG_verts__m_edge_idx":       [0, 2, 1],
    "gpu___CG_p_patch__CG_verts__m_edge_blk":       [0, 2, 1],
    "gpu___CG_p_patch__CG_edges__m_cell_idx":       [0, 2, 1],
    "gpu___CG_p_patch__CG_edges__m_cell_blk":       [0, 2, 1],
    "gpu___CG_p_patch__CG_cells__m_edge_idx":       [0, 2, 1],
    "gpu___CG_p_patch__CG_cells__m_edge_blk":       [0, 2, 1],
    "gpu___CG_p_patch__CG_edges__m_vertex_idx":     [0, 2, 1],
    "gpu___CG_p_patch__CG_edges__m_vertex_blk":     [0, 2, 1],
    "gpu___CG_p_patch__CG_edges__m_quad_idx":       [0, 2, 1],
    "gpu___CG_p_patch__CG_edges__m_quad_blk":       [0, 2, 1],
    "gpu___CG_p_patch__CG_cells__m_neighbor_idx":   [0, 2, 1],
    "gpu___CG_p_patch__CG_cells__m_neighbor_blk":   [0, 2, 1],
}

_CURATED_INDEX_ONLY: PermMap = {
    "gpu___CG_p_int__m_e_bln_c_s":                  [0, 1, 2],
    "gpu___CG_p_int__m_c_lin_e":                    [0, 1, 2],
    "gpu___CG_p_int__m_geofac_n2s":                 [0, 1, 2],
    "gpu___CG_p_int__m_geofac_grdiv":               [0, 1, 2],
    "gpu___CG_p_int__m_cells_aw_verts":             [0, 1, 2],
    "gpu___CG_p_int__m_geofac_rot":                 [0, 1, 2],
    "gpu___CG_p_int__m_rbf_vec_coeff_e":            [0, 1, 2],
    "gpu___CG_p_patch__CG_verts__m_cell_blk":       [0, 2, 1],
    "gpu___CG_p_patch__CG_verts__m_cell_idx":       [0, 2, 1],
    "gpu___CG_p_patch__CG_verts__m_edge_idx":       [0, 2, 1],
    "gpu___CG_p_patch__CG_verts__m_edge_blk":       [0, 2, 1],
    "gpu___CG_p_patch__CG_edges__m_cell_idx":       [0, 2, 1],
    "gpu___CG_p_patch__CG_edges__m_cell_blk":       [0, 2, 1],
    "gpu___CG_p_patch__CG_cells__m_edge_idx":       [0, 2, 1],
    "gpu___CG_p_patch__CG_cells__m_edge_blk":       [0, 2, 1],
    "gpu___CG_p_patch__CG_edges__m_vertex_idx":     [0, 2, 1],
    "gpu___CG_p_patch__CG_edges__m_vertex_blk":     [0, 2, 1],
    "gpu___CG_p_patch__CG_edges__m_quad_idx":       [0, 2, 1],
    "gpu___CG_p_patch__CG_edges__m_quad_blk":       [0, 2, 1],
    "gpu___CG_p_patch__CG_cells__m_neighbor_idx":   [0, 2, 1],
    "gpu___CG_p_patch__CG_cells__m_neighbor_blk":   [0, 2, 1],
}


def curated_configs(sdfg_arrays: Dict[str, int] | None = None) -> List[PermuteConfig]:
    """Return the full curated config list.

    Layout: 94 sweep cells (``cv<0|1>_ch<0|1>_f<0|1>_s<0|1>_n<perm>``,
    minus the all-zero / identity-conn cell which would duplicate
    ``unpermuted``) followed by ``curated_nlev_first`` and
    ``curated_index_only``. ``sdfg_arrays`` is the same ``{name: ndim}``
    map used by ``configs_from_candidates`` -- when supplied, entries
    whose dimensionality doesn't match the permutation are silently
    dropped instead of failing inside ``PermuteDimensions``.
    """
    out: List[PermuteConfig] = []

    for cv in (0, 1):
        for ch in (0, 1):
            for f in (0, 1):
                for s in (0, 1):
                    for np_perm in _ALL_CONN_PERMS:
                        for em in (0, 1):
                            for lv in (0, 1):
                                for sm in (0, 1):
                                    if (cv == 0 and ch == 0 and f == 0
                                            and s == 0 and np_perm == _ID_CONN
                                            and em == 0 and lv == 0 and sm == 0):
                                        # All-zero sweep cell ==
                                        # unpermuted_em0_lv0_sm0; skip to avoid
                                        # duplicating the heuristic config.
                                        continue
                                    pmap = _build_sweep_map(cv, ch, f, s,
                                                            np_perm, em, lv)
                                    pmap = _filter_by_dim(pmap, sdfg_arrays)
                                    if not pmap and sm == 0:
                                        # Empty permute_map AND no map shuffle
                                        # = nothing to do; skip. (Empty +
                                        # sm=1 still does the loop reorder.)
                                        continue
                                    out.append(PermuteConfig(
                                        name=_cfg_name(cv, ch, f, s, np_perm,
                                                       em, lv, sm),
                                        permute_map=pmap,
                                        shuffle_map=bool(sm),
                                    ))

    nlev_filtered = _filter_by_dim(_CURATED_NLEV_FIRST, sdfg_arrays)
    if nlev_filtered:
        out.append(PermuteConfig(name="curated_nlev_first", permute_map=nlev_filtered))

    idx_filtered = _filter_by_dim(_CURATED_INDEX_ONLY, sdfg_arrays)
    if idx_filtered:
        out.append(PermuteConfig(name="curated_index_only", permute_map=idx_filtered))

    return out
