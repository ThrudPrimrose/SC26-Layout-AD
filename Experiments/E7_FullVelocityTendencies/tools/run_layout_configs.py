#!/usr/bin/env python3
"""
run_layout_configs.py

Launcher: drive E7's stage-6 optimize+compile pipeline over every layout
config in E6's ``layout_crossproduct_v123.json`` (produced by
``E6_VelocityTendencies/generate_v123_candidates.py``).

Companion to ``tools/list_layout_configs.py``: that one lists the named
configs available to ``stage6 --config``; this one *runs* the
group-cross-product configs end-to-end.

Each V-id config gets translated into a ``PermuteConfig`` per the
following rules (consistent with the V-id projection in
``_analysis_util.py``):

  IC-axis groups (cv / ch / f / s / lm):
      V1, V2          -> identity (no permutation)
      V3, V4, V5, V6  -> nlev-first
                          [1, 0, 2] for 3-D arrays
                          [1, 0]    for 2-D arrays
                          [1, 0, 2, 3] for 4-D arrays (e.g. p_diag*pc)

  IN-axis group (n):
      V_odd  (V1, V3, V5, V7) -> identity (SoA)
      V_even (V2, V4, V6)     -> AoS [0, 2, 1] (the empirical winner)

The 729 V-id combinations (3**6) collapse to 64 unique permutation maps
(2**4 IC binary * 2 IN binary). Duplicate signatures are run only once;
later V-id names that hit the same permutation are skipped silently.

Usage:
    python tools/run_layout_configs.py
    python tools/run_layout_configs.py --optimize --compile
    python tools/run_layout_configs.py --config v123_cv_V6_ch_V1_f_V6_s_V6_n_V6_lm_V6
    python tools/run_layout_configs.py --v123-json /path/to/layout_crossproduct_v123.json
    python tools/run_layout_configs.py --dry-run        # list configs without running
"""

import argparse
import copy
import json
import sys
from pathlib import Path
from typing import Dict, List

# Make E7's utils/ importable when run from anywhere.
_E7_ROOT = Path(__file__).resolve().parent.parent
if str(_E7_ROOT) not in sys.path:
    sys.path.insert(0, str(_E7_ROOT))

# Inline copy of fortran_to_sdfg_array_name from utils/passes/permute_configs.py
# so this module can dry-run without importing dace. The full sweep
# (--optimize / --compile) imports dace lazily inside main().
def fortran_to_sdfg_array_name(s: str) -> str:
    if "%" not in s:
        return s
    return "__CG_" + s.replace("%", "__m_").strip()


STAGE_ID = 6

# Default JSON path: E6's full_velocity_tendencies/ output.
_E6_DIR = _E7_ROOT.parent / "E6_VelocityTendencies"
DEFAULT_V123_JSON = _E6_DIR / "full_velocity_tendencies" / "layout_crossproduct_v123.json"
DEFAULT_GROUPS_JSON = _E6_DIR / "access_analysis" / "canonical_array_groups.json"


# ──────────────────────────────────────────────────────────────────────
#  V-id translation
# ──────────────────────────────────────────────────────────────────────

def _v_id_num(v: str) -> int | None:
    if not v.startswith("V") or not v[1:].isdigit():
        return None
    return int(v[1:])


def v_ic_is_v_first(v: str) -> bool:
    """V-id projection on the IC axis: V1, V2 -> h_first; V>=3 -> v_first."""
    n = _v_id_num(v)
    return n is not None and n >= 3


def v_in_is_aos(v: str) -> bool:
    """V-id projection on the IN axis: V_odd -> SoA; V_even -> AoS."""
    n = _v_id_num(v)
    return n is not None and n % 2 == 0


# Permutations for the v_first / AoS arms.
_NLEV_FIRST_BY_DIM = {2: [1, 0], 3: [1, 0, 2], 4: [1, 0, 2, 3]}
_AOS_3D = [0, 2, 1]


# ──────────────────────────────────────────────────────────────────────
#  cfg -> PermuteConfig
# ──────────────────────────────────────────────────────────────────────

def cfg_label(cfg: dict, group_ids: List[str]) -> str:
    """Build a human-readable config name from V-id assignments."""
    parts = [f"{gid}_{cfg[gid]}" for gid in group_ids]
    return "v123_" + "_".join(parts)


def build_permute_map(cfg: dict, group_arrays: Dict[str, List[str]],
                      group_axis: Dict[str, str],
                      sdfg_arrays: Dict[str, int]) -> Dict[str, List[int]]:
    """Translate a V-id config into a {sdfg_array_name: permutation} map.

    Skips arrays not present in ``sdfg_arrays`` and arrays whose
    dimensionality has no defined nlev-first permutation."""
    pmap: Dict[str, List[int]] = {}

    for gid, arrs in group_arrays.items():
        v = cfg.get(gid, "V1")
        axis = group_axis.get(gid, "IC")
        for fortran_name in arrs:
            sdfg_name = fortran_to_sdfg_array_name(fortran_name)
            ndim = sdfg_arrays.get(sdfg_name)
            if ndim is None:
                continue
            if axis == "IC":
                if v_ic_is_v_first(v):
                    perm = _NLEV_FIRST_BY_DIM.get(ndim)
                    if perm is not None:
                        pmap[sdfg_name] = list(perm)
            elif axis == "IN":
                if v_in_is_aos(v) and ndim == 3:
                    pmap[sdfg_name] = list(_AOS_3D)
            # silently skip unknown axes

    return pmap


def signature(pmap: Dict[str, List[int]]) -> str:
    """Stable hash of a permute map for dedup."""
    return json.dumps(pmap, sort_keys=True)


# ──────────────────────────────────────────────────────────────────────
#  main
# ──────────────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--v123-json", type=Path, default=DEFAULT_V123_JSON,
                    help=f"path to layout_crossproduct_v123.json "
                         f"(default: {DEFAULT_V123_JSON})")
    ap.add_argument("--groups-json", type=Path, default=DEFAULT_GROUPS_JSON,
                    help=f"path to canonical_array_groups.json "
                         f"(default: {DEFAULT_GROUPS_JSON})")
    ap.add_argument("--optimize", action=argparse.BooleanOptionalAction, default=False)
    ap.add_argument("--compile", action=argparse.BooleanOptionalAction, default=False)
    ap.add_argument("--config", type=str, default=None,
                    help="run only the named config (default: every unique permute-map)")
    ap.add_argument("--debug", dest="release", action="store_false", default=True,
                    help="build with -O0 + DACE_VELOCITY_DEBUG (default: release)")
    ap.add_argument("--dry-run", action="store_true",
                    help="list the configs that would run; don't compile")
    args = ap.parse_args()
    if not args.optimize and not args.compile and not args.dry_run:
        args.optimize = args.compile = True

    # Lazy import: only needed when actually compiling. Dry-run skips this.
    if not args.dry_run:
        global dace, _apply_force_permuted, PermuteConfig, permute_layout
        global common, _array_dim_map, _sdfg_uses_gpu
        import dace  # noqa: F401
        from utils.passes.permute_configs import _apply_force_permuted  # noqa: F401
        from utils.passes.permute_layout import PermuteConfig, permute_layout  # noqa: F401
        from utils.stages import common  # noqa: F401
        from utils.stages.stage5a import _array_dim_map, _sdfg_uses_gpu  # noqa: F401

    cross = json.loads(args.v123_json.read_text())
    groups = json.loads(args.groups_json.read_text())
    group_ids = groups["group_ids"]
    group_arrays = groups["arrays"]
    group_axis = groups["group_axis"]
    configs_v123 = cross["crossproduct"]

    print(f"[run_layout_configs] {len(configs_v123)} V-id configs from "
          f"{args.v123_json.name}")
    print(f"  groups = {group_ids}")

    repo_root = _E7_ROOT

    # Dry-run path: enumerate unique permute-map signatures without
    # touching any SDFG.
    if args.dry_run:
        seen: Dict[str, str] = {}
        # Use a "ndim probe" map that pretends every array exists at
        # 3-D so we get the most-permuted variant — useful enough for
        # a config preview. The real run reads dims from each SDFG.
        all_arrs: Dict[str, int] = {}
        for arrs in group_arrays.values():
            for a in arrs:
                all_arrs[fortran_to_sdfg_array_name(a)] = 3
        unique = []
        for cfg in configs_v123:
            cname = cfg_label(cfg, group_ids)
            if args.config is not None and cname != args.config:
                continue
            pmap = build_permute_map(cfg, group_arrays, group_axis, all_arrs)
            sig = signature(pmap)
            first = seen.setdefault(sig, cname)
            if first != cname:
                continue
            unique.append((cname, pmap))
        print(f"  unique permute-map signatures: {len(unique)} "
              f"(vs {len(configs_v123)} raw)")
        for cname, pmap in unique:
            print(f"  [{cname}] {len(pmap)} array permutation(s)")
        return 0

    names = common.sdfg_names()

    # ── optimize: deepcopy + permute + save per config ──
    if args.optimize or args.dry_run:
        first_sdfg = None
        sdfg_arr_dims_by_name: Dict[str, Dict[str, int]] = {}
        for name in names:
            infile = common.stage_input(name, STAGE_ID)
            if not args.dry_run:
                print(f"Stage #{STAGE_ID}: loading {name} from {infile}")
            sdfg = dace.SDFG.from_file(infile)
            sdfg.name = name

            if not _sdfg_uses_gpu(sdfg):
                if not args.dry_run:
                    print(f"  [skip] {name}: no GPU_Device map; "
                          f"permutations are GPU-only")
                continue

            sdfg_arr_dims_by_name[name] = _array_dim_map(sdfg)
            if first_sdfg is None:
                first_sdfg = sdfg

        if first_sdfg is None:
            print("error: no GPU SDFG found; nothing to do")
            return 1

        # Use the union of dims across variants for map building (stage6
        # itself filters per-SDFG when applying).
        union_dims: Dict[str, int] = {}
        for d in sdfg_arr_dims_by_name.values():
            union_dims.update(d)

        seen_signatures: Dict[str, str] = {}  # sig -> first_cname
        unique_configs: List[tuple[str, Dict[str, List[int]]]] = []
        for cfg in configs_v123:
            cname = cfg_label(cfg, group_ids)
            if args.config is not None and cname != args.config:
                continue
            pmap = build_permute_map(cfg, group_arrays, group_axis, union_dims)
            sig = signature(pmap)
            first = seen_signatures.setdefault(sig, cname)
            if first != cname:
                # duplicate of an earlier V-id combo with the same effect
                continue
            unique_configs.append((cname, pmap))

        print(f"  unique permute-map signatures: {len(unique_configs)} "
              f"(vs {len(configs_v123)} raw configs)")

        if args.dry_run:
            for cname, pmap in unique_configs:
                print(f"  [{cname}] {len(pmap)} array permutation(s)")
            return 0

        # Apply the permutations to each SDFG variant.
        for name in names:
            if name not in sdfg_arr_dims_by_name:
                continue
            sdfg = dace.SDFG.from_file(common.stage_input(name, STAGE_ID))
            sdfg.name = name
            sdfg_arr_dims = sdfg_arr_dims_by_name[name]

            for cname, pmap_union in unique_configs:
                # Restrict to arrays present in THIS variant.
                pmap = {k: v for k, v in pmap_union.items()
                        if sdfg_arr_dims.get(k) == len(v)}
                pc = PermuteConfig(name=cname, permute_map=pmap)
                pc, = _apply_force_permuted([pc], sdfg_arr_dims)

                permuted = copy.deepcopy(sdfg)
                permuted.name = name
                count = permute_layout(permuted, pc, shuffle_map=pc.shuffle_map)
                print(f"  [{cname}] {name}: permuted {count} array(s)")

                outfile = (repo_root / common.CODEGEN_DIR / f"stage{STAGE_ID}"
                           / cname / f"{name}.sdfgz")
                outfile.parent.mkdir(parents=True, exist_ok=True)
                permuted.save(str(outfile), compress=True)

    # ── compile: invoke common.compile_action per unique config ──
    if args.compile:
        seen_signatures: Dict[str, str] = {}
        unique_names: List[str] = []
        # Re-derive unique config names without needing SDFG dims.
        for cfg in configs_v123:
            cname = cfg_label(cfg, group_ids)
            if args.config is not None and cname != args.config:
                continue
            # Use a None-dim build_permute_map proxy for signature: we
            # can reconstruct using union dims from the optimize step;
            # if optimize wasn't run (compile-only), recompute from disk.
            stage6_dir = repo_root / common.CODEGEN_DIR / f"stage{STAGE_ID}" / cname
            if not stage6_dir.is_dir():
                continue
            unique_names.append(cname)

        # Deduplicate by directory contents (each unique permute-map
        # was saved once during --optimize).
        seen_dirs: set = set()
        for cname in unique_names:
            stage6_dir = repo_root / common.CODEGEN_DIR / f"stage{STAGE_ID}" / cname
            if cname in seen_dirs:
                continue
            seen_dirs.add(cname)
            sdfgs: Dict[str, dace.SDFG] = {}
            for name in names:
                p = stage6_dir / f"{name}.sdfgz"
                if p.exists():
                    sdfgs[name] = dace.SDFG.from_file(str(p))
            if not sdfgs:
                print(f"  [skip-compile] {cname}: no SDFGs (run --optimize first)")
                continue
            common.compile_action(
                STAGE_ID, sdfgs, gpu=True, release=args.release,
                output=f"velocity_stage{STAGE_ID}_{cname}",
                extra_sources=["src/reductions.cpp", "src/reductions_kernel.cu"],
                extra_include_dirs=["include"],
            )

    return 0


if __name__ == "__main__":
    sys.exit(main())
