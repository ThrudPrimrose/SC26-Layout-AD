#!/usr/bin/env python3
"""Emit ``layout_groups.json`` -- the auditable spec for E7's V_k winners.

Reviewers reading the AD trace claims about the velocity-tendencies
sweep through three artifacts:

  1. paper §IV-A / §IV-D: "5 compute patterns + level reduction;
     7 layout-equivalent groups; top-k per group; Cartesian sweep"
  2. ``layout_candidates.json``  -- selection of representative
     loop nests per pattern class (this directory's other emitter,
     ``select_loopnests.py``)
  3. ``layout_groups.json``      -- the per-group V_k spec emitted
     by THIS script. Defines the 7 layout-equivalent groups, the
     V1/V2/V6 permutation each group takes, and which arrays
     belong to each group.

E7's ``utils/passes/permute_configs.py`` reads ``layout_groups.json``
to build:

  * ``winner_v1``, ``winner_v2``, ``winner_v6``  -- uniform V_k applied
    across every group; corresponds to Fig. 13 ``Original Layout``
    (V1) and ``Optimized Layout`` (V6), with V2 as the connectivity-
    only-permuted intermediate.
  * Cross-product sweep ``cv*_ch*_f*_s*_n*_em*_lv*_sm*`` -- one cell
    per per-group V_k assignment. Each cell is doubled with sm0/sm1
    (no map shuffle / map shuffle). Connectivity (n) sweeps all six
    permutations rather than just V1/V2/V6 so the three unlabelled
    perms are confirmed losers, not assumed.

Run:
    python generate_layout_groups.py

Output:
    layout_groups.json next to this script.
"""
from __future__ import annotations

import json
from itertools import permutations as _iter_perms
from pathlib import Path

HERE = Path(__file__).resolve().parent
OUT_PATH = HERE / "layout_groups.json"


# V_k permutations per group. The V_k labels follow E6's
# `_analysis_util.py` storage-axis projection: V1 = identity, V2 =
# h_first + AoS-conn (only meaningful for index/connectivity arrays;
# data groups inherit V1's identity), V6 = v_first + AoS-conn (data
# is level-first; connectivity has the short dim N innermost).
#
# Every PerLine here is the STRICT V_k permutation. The data groups
# have V2 == V1 because the V2 vs V1 distinction is purely in the
# connectivity layout (the N-dim axis), and data groups don't have
# an N-dim axis.

_GROUPS = [
    {
        "name": "cv",
        "doc": "rank-3 vertical-accumulation work arrays "
               "(jk is the natural parallelism axis; V6 = level-first)",
        "rank": 3,
        "vk_perms": {
            "V1": [0, 1, 2],
            "V2": [0, 1, 2],
            "V6": [1, 0, 2],
        },
        "arrays": [
            "gpu_z_w_con_c_full",
            "gpu_z_w_concorr_me",
            "gpu_z_w_v",
            "gpu_z_w_concorr_mc",
            "gpu_z_w_con_c",
        ],
    },
    {
        "name": "ch",
        "doc": "rank-3 horizontal-stencil work arrays "
               "(je/jc innermost; V6 = level-first)",
        "rank": 3,
        "vk_perms": {
            "V1": [0, 1, 2],
            "V2": [0, 1, 2],
            "V6": [1, 0, 2],
        },
        "arrays": [
            "gpu_z_ekinh",
            "gpu_z_kin_hor_e",
            "gpu_z_vt_ie",
            "gpu_z_v_grad_w",
            "gpu_zeta",
            "gpu_maxvcfl",
            "gpu_cfl_clipping",
        ],
    },
    {
        "name": "f",
        "doc": "rank-3 prog/diag fields + rank-4 ddt fields "
               "(ddt's trailing ntl always stays last)",
        # "rank" is the dominant rank; rank-4 entries (ddt_*) carry
        # their own rank-4 V6 permutation in vk_perms_rank4. Helps
        # the consumer pick the correct perm at array-rewrite time.
        "rank": 3,
        "vk_perms": {
            "V1": [0, 1, 2],
            "V2": [0, 1, 2],
            "V6": [1, 0, 2],
        },
        "vk_perms_rank4": {
            "V1": [0, 1, 2, 3],
            "V2": [0, 1, 2, 3],
            "V6": [1, 0, 2, 3],
        },
        "arrays": [
            "gpu___CG_p_prog__m_vn",
            "gpu___CG_p_prog__m_w",
            "gpu___CG_p_diag__m_vt",
            "gpu___CG_p_diag__m_vn_ie",
            "gpu___CG_p_diag__m_vn_ie_ubc",
            "gpu___CG_p_diag__m_w_concorr_c",
        ],
        "arrays_rank4": [
            "gpu___CG_p_diag__m_ddt_vn_apc_pc",
            "gpu___CG_p_diag__m_ddt_w_adv_pc",
            "gpu___CG_p_diag__m_ddt_vn_cor_pc",
        ],
    },
    {
        "name": "s",
        "doc": "rank-3 read-only metric + interpolation coefficients",
        "rank": 3,
        "vk_perms": {
            "V1": [0, 1, 2],
            "V2": [0, 1, 2],
            "V6": [1, 0, 2],
        },
        "arrays": [
            "gpu___CG_p_metrics__m_wgtfac_e",
            "gpu___CG_p_metrics__m_wgtfacq_e",
            "gpu___CG_p_metrics__m_wgtfac_c",
            "gpu___CG_p_metrics__m_ddxn_z_full",
            "gpu___CG_p_metrics__m_ddxt_z_full",
            "gpu___CG_p_metrics__m_ddqz_z_half",
            "gpu___CG_p_metrics__m_ddqz_z_full_e",
            "gpu___CG_p_metrics__m_coeff1_dwdz",
            "gpu___CG_p_metrics__m_coeff2_dwdz",
            "gpu___CG_p_metrics__m_coeff_gradekin",
            "gpu___CG_p_int__m_e_bln_c_s",
            "gpu___CG_p_int__m_c_lin_e",
            "gpu___CG_p_int__m_geofac_n2s",
            "gpu___CG_p_int__m_geofac_grdiv",
            "gpu___CG_p_int__m_cells_aw_verts",
            "gpu___CG_p_int__m_geofac_rot",
            "gpu___CG_p_int__m_rbf_vec_coeff_e",
        ],
    },
    {
        "name": "lv",
        "doc": "rank-2 gpu_levmask (the only (nblks, nlev) array)",
        "rank": 2,
        "vk_perms": {
            "V1": [0, 1],
            "V2": [0, 1],
            "V6": [1, 0],
        },
        "arrays": [
            "gpu_levmask",
        ],
    },
    {
        "name": "em",
        "doc": "rank-2 edge / cell scalar metrics; untouched by E6's "
               "loop-nest analysis (no nlev / nproma access "
               "structure), included for completeness",
        "rank": 2,
        "vk_perms": {
            "V1": [0, 1],
            "V2": [0, 1],
            "V6": [1, 0],
        },
        "arrays": [
            "gpu___CG_p_patch__CG_edges__m_area_edge",
            "gpu___CG_p_patch__CG_edges__m_f_e",
            "gpu___CG_p_patch__CG_edges__m_inv_primal_edge_length",
            "gpu___CG_p_patch__CG_edges__m_inv_dual_edge_length",
            "gpu___CG_p_patch__CG_edges__m_tangent_orientation",
            "gpu___CG_p_patch__CG_cells__m_area",
        ],
    },
    {
        "name": "n",
        "doc": "rank-3 connectivity index arrays (nproma, nblks, N). "
               "V2 = (nproma, N, nblks); V6 = (N, nproma, nblks). The "
               "sweep enumerates all six permutations of [0, 1, 2]; "
               "V1/V2/V6 are the three for which a paper claim exists, "
               "the other three confirm they don't beat V6.",
        "rank": 3,
        "vk_perms": {
            "V1": [0, 1, 2],
            "V2": [0, 2, 1],
            "V6": [2, 0, 1],
        },
        "sweep_all_perms": True,
        "all_perms": [list(p) for p in _iter_perms([0, 1, 2])],
        "arrays": [
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
        ],
    },
]


_NAMED_CONFIGS = {
    "winner_v1": {
        "doc": "Fig. 13 'Original Layout' baseline. "
               "Uniform V1 across every group = identity = no permutation.",
        "uniform_vk": "V1",
    },
    "winner_v2": {
        "doc": "Connectivity-only intermediate. Uniform V2: data "
               "groups inherit V1 identity (V2 == V1 storage-wise for "
               "data); the connectivity group is permuted to "
               "(nproma, N, nblks).",
        "uniform_vk": "V2",
    },
    "winner_v6": {
        "doc": "Fig. 13 'Optimized Layout' adopted in §IV-D. Uniform "
               "V6: data groups go level-first; connectivity goes "
               "(N, nproma, nblks). This is what stage 5a runs by "
               "default for the V6 paper claim.",
        "uniform_vk": "V6",
    },
}


_SWEEP_SPEC = {
    "doc": "Cross-product sweep over the seven groups. Each group's "
           "axis is the V_k options listed above; for groups with "
           "sweep_all_perms=True, the axis enumerates every permutation "
           "of the group's rank rather than just the named V_k. Each "
           "resulting cell is doubled by the sm (loop-shuffle) axis.",
    "groups": [g["name"] for g in _GROUPS],
    "loop_shuffle": [False, True],
    "axis_naming": "cv<i>_ch<i>_f<i>_s<i>_n<perm>_em<i>_lv<i>_sm<i>",
}


def _payload() -> dict:
    return {
        "schema_version": 1,
        "doc": "Auditable spec for E7 V_k winner generation. Read by "
               "Experiments/E7_FullVelocityTendencies/utils/passes/"
               "permute_configs.py to build winner_v1 / winner_v2 / "
               "winner_v6 plus the cross-product sweep cells.",
        "groups": _GROUPS,
        "named_configs": _NAMED_CONFIGS,
        "sweep": _SWEEP_SPEC,
    }


def main() -> None:
    OUT_PATH.write_text(json.dumps(_payload(), indent=2) + "\n")
    print(f"wrote {OUT_PATH}")
    arrays_total = sum(
        len(g.get("arrays", [])) + len(g.get("arrays_rank4", []))
        for g in _GROUPS
    )
    print(f"  groups={len(_GROUPS)}  arrays_total={arrays_total}  "
          f"named_configs={len(_NAMED_CONFIGS)}")


if __name__ == "__main__":
    main()
