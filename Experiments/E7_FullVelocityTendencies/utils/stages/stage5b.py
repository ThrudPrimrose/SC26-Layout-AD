"""Stage 5b: neighborhood-list / block-array compression (alternative to 5a).

Carries the velocity-tendencies compression work plus an always-on
``levmask`` transpose. Other per-array permutations are optional and
read from ``layout_candidates.json`` (E6's access-analysis output)
when ``--config <name>`` is given.

Stage 5a and stage 5b are mutually exclusive paths off stage 4: 5a
adds permutations only, 5b adds compression. Pick whichever matches
the experiment.

Three compression call sites:

1. :func:`fold_array_access_to_expression` on the ``*_blk`` arrays
   known to be single-valued under the runtime precondition
   ``nblks_c == 1 && nblks_v == 1``. Every subscript access is
   rewritten to the literal 1 (Fortran 1-indexed block ID); arrays
   themselves stay in the function signature, just unused.
2. :func:`generate_compressed_variant` on the ``*_idx`` arrays with
   ``uint16`` under the bound ``nproma * nblks_* < 65536``. Adds a
   compressed clone of the body and dispatches at runtime.
3. :func:`generate_compressed_variant` on the multi-valued ``*_blk``
   arrays with ``uint8`` under the bound ``nblks_* < 256``.

Forced layout rewrite:

* ``levmask`` / ``gpu_levmask`` are always permuted with ``[1, 0]``
  (transpose). The Fortran source declares ``levmask(nblks_c, nlev)``
  (column-major: ``jb`` fast-varying), but f2dace lowers it as C
  row-major ``levmask[jb, jk]``. Transposing restores the
  Fortran-canonical layout AND fixes the stride for the
  ``ANY(levmask(:, jk))`` reduce. Empirically uniformly better.

Run:
    python -m utils.stages.stage5b              # optimise + compile
    python -m utils.stages.stage5b --optimize   # codegen stage5b .sdfgz
    python -m utils.stages.stage5b --compile    # compile existing stage5b
    python -m utils.stages.stage5b --config nlev_first    # merge JSON config
"""

import argparse
import copy
import os
from pathlib import Path
from typing import Dict, List

os.environ.setdefault("__DACE_NO_SYNC", "1")

from utils.dace_branch import YAKUP_DEV_BRANCH, ensure_branch
ensure_branch(YAKUP_DEV_BRANCH)

import dace

from utils.passes.compress_indices import (
    SymbolicConstraint,
    fold_array_access_to_expression,
    generate_compressed_variant,
)
from utils.passes.permute_configs import extended_configs_from_candidates
from utils.passes.permute_layout import PermuteConfig, permute_layout
from utils.stages import common


STAGE_ID = "5b"
# stage 5b reads from stage 4's output, regardless of the string id.
_INPUT_STAGE = 5  # ``stage_input(name, 5)`` -> ``codegen/stage4/<name>.sdfgz``


# ---------- Forced ``levmask`` transpose --------------------------------

# The Fortran source declares ``levmask(nblks_c, nlev)`` (column-major,
# ``jb`` fast-varying); f2dace's row-major lowering inverts that. The
# ``ANY(levmask(:, jk))`` reduce on line 600 wants ``jb`` contiguous, so
# we transpose to ``[1, 0]`` -- i.e. ``levmask[jk, jb]``. Both the
# host-side ``levmask`` and the GPU mirror ``gpu_levmask`` are listed:
# stage 4's ``OffloadVelocityToGPU`` mirrors host arrays into
# ``gpu_<name>`` siblings, and post-stage-4 SDFGs reference the GPU
# name only. The bare ``levmask`` entry stays for callers that run
# this stage on a pre-stage-4 SDFG (e.g. test fixtures).
_FORCED_PERMUTE: Dict[str, List[int]] = {
    "levmask": [1, 0],
    "gpu_levmask": [1, 0],
}


# ---------- Compression knobs ------------------------------------------

# Neighborhood-list indices demoted to ``uint16`` under the bound
# ``nproma * nblks_* < 65536`` at runtime.
_IDX_ARRAYS_GPU = (
    "gpu___CG_p_patch__CG_cells__m_edge_idx",
    "gpu___CG_p_patch__CG_cells__m_neighbor_idx",
    "gpu___CG_p_patch__CG_edges__m_cell_idx",
    "gpu___CG_p_patch__CG_edges__m_quad_idx",
    "gpu___CG_p_patch__CG_edges__m_vertex_idx",
    "gpu___CG_p_patch__CG_verts__m_cell_idx",
    "gpu___CG_p_patch__CG_verts__m_edge_idx",
)

# Block-index arrays whose every value equals the enclosing block-loop
# iterator ``jb`` (structural on single-block grids). Folded to the
# subscript expression -- arrays dropped entirely post-rewrite.
_BLK_ARRAYS_SINGLE_VAL_GPU = (
    "gpu___CG_p_patch__CG_cells__m_neighbor_blk",
    "gpu___CG_p_patch__CG_edges__m_cell_blk",
    "gpu___CG_p_patch__CG_edges__m_vertex_blk",
    "gpu___CG_p_patch__CG_verts__m_cell_blk",
)

# Block-index arrays that do carry distinct block numbers. Demoted to
# ``uint8`` under ``nblks_* < 256``.
_BLK_ARRAYS_MULTI_VAL_GPU = (
    "gpu___CG_p_patch__CG_cells__m_edge_blk",
    "gpu___CG_p_patch__CG_edges__m_quad_blk",
    "gpu___CG_p_patch__CG_verts__m_edge_blk",
)

# Runtime preconditions for the compression rewrites.
_SINGLE_BLOCK = SymbolicConstraint(
    "__CG_p_patch__m_nblks_c == 1 && "
    "__CG_p_patch__m_nblks_v == 1")

_IDX_FITS_UINT16 = SymbolicConstraint(
    "__CG_global_data__m_nproma * __CG_p_patch__m_nblks_c < 65536 && "
    "__CG_global_data__m_nproma * __CG_p_patch__m_nblks_e < 65536 && "
    "__CG_global_data__m_nproma * __CG_p_patch__m_nblks_v < 65536")

_BLK_FITS_UINT8 = SymbolicConstraint(
    "__CG_p_patch__m_nblks_c < 256 && "
    "__CG_p_patch__m_nblks_e < 256 && "
    "__CG_p_patch__m_nblks_v < 256")


# ---------- Per-stage-5b transformation --------------------------------

def _build_permute_config(
    forced: Dict[str, List[int]],
    json_path: Path | None,
    config_name: str | None,
    sdfg_arrays: Dict[str, int],
) -> PermuteConfig:
    """Return one ``PermuteConfig`` that combines:

    - the forced ``levmask`` transpose (always present), and
    - any per-array permutations from the named JSON config (if given).

    JSON entries override the forced map for the same array name only
    when ``config_name`` actually provides one for it -- so
    ``levmask`` stays transposed unless the JSON explicitly disagrees.
    """
    permute_map: Dict[str, List[int]] = {}
    if json_path is not None and config_name is not None:
        configs = extended_configs_from_candidates(json_path, sdfg_arrays=sdfg_arrays)
        match = [c for c in configs if c.name == config_name]
        if not match:
            known = ", ".join(c.name for c in configs)
            raise SystemExit(
                f"Stage #{STAGE_ID}: no config named '{config_name}' "
                f"(known: {known})"
            )
        permute_map.update(match[0].permute_map)
    # Forced overrides win (the JSON heuristic doesn't know about
    # levmask's stride bug -- forcing here makes the layout uniform
    # regardless of which JSON entry was selected).
    for name, perm in forced.items():
        ndim = sdfg_arrays.get(name)
        if ndim is None or ndim != len(perm):
            continue
        permute_map[name] = list(perm)
    label = "levmask_only" if config_name is None else f"levmask_plus_{config_name}"
    return PermuteConfig(name=label, permute_map=permute_map)


def optimization_action(sdfg: dace.SDFG,
                        json_path: Path | None,
                        config_name: str | None) -> dace.SDFG:
    # 1. Single-val blk elimination via fold.
    eliminated = fold_array_access_to_expression(
        sdfg,
        array_names=_BLK_ARRAYS_SINGLE_VAL_GPU,
        rewrite_rule=lambda _name, _idxs: 1,
        constraints=[_SINGLE_BLOCK],
    )
    if eliminated:
        print(f"Stage #{STAGE_ID}: fold_array_access_to_expression "
              f"eliminated {eliminated} blk array(s)")
    sdfg.validate()

    # 2. Neighbor-index uint16 demotion.
    generate_compressed_variant(
        sdfg, array_names=_IDX_ARRAYS_GPU,
        target_dtype=dace.uint16,
        constraints=[_SINGLE_BLOCK, _IDX_FITS_UINT16],
        name_suffix='uint16')
    sdfg.validate()

    # 3. Multi-val blk uint8 demotion.
    generate_compressed_variant(
        sdfg, array_names=_BLK_ARRAYS_MULTI_VAL_GPU,
        target_dtype=dace.uint8,
        constraints=[_SINGLE_BLOCK, _BLK_FITS_UINT8],
        name_suffix='uint8')
    sdfg.validate()

    # 4. Forced ``levmask`` transpose + optional JSON-driven permutations.
    sdfg_arrays = {n: len(a.shape) for n, a in sdfg.arrays.items()}
    cfg = _build_permute_config(_FORCED_PERMUTE, json_path, config_name, sdfg_arrays)
    if cfg.permute_map:
        count = permute_layout(sdfg, cfg, shuffle_map=True)
        names_str = ", ".join(sorted(cfg.permute_map))
        print(f"Stage #{STAGE_ID}: permute_layout [{cfg.name}] permuted "
              f"{count} array(s) (target arrays: {names_str})")
        sdfg.validate()

    return sdfg


# ---------- CLI / driver ------------------------------------------------

def main():
    repo_root = Path(__file__).resolve().parent.parent.parent
    default_json = (
        Path("/home/primrose/Work/SC26-Layout-AD")
        / "Experiments" / "E6_VelocityTendencies"
        / "access_analysis" / "layout_candidates.json"
    )

    argp = argparse.ArgumentParser()
    argp.add_argument("--optimize", action=argparse.BooleanOptionalAction, default=False)
    argp.add_argument("--compile", action=argparse.BooleanOptionalAction, default=False)
    argp.add_argument("--config", type=str, default=None,
                      help="optional JSON config name to merge with the "
                           "always-on levmask transpose (default: levmask alone)")
    argp.add_argument("--candidates", type=Path, default=default_json,
                      help="path to layout_candidates.json")
    mode = argp.add_mutually_exclusive_group()
    mode.add_argument("--release", dest="release", action="store_true",
                      help="build with -O3 + --use_fast_math (FMA may diverge from IEEE)")
    mode.add_argument("--debug", dest="release", action="store_false",
                      help="build with -O0 + IEEE fp (default)")
    argp.set_defaults(release=False)
    args = argp.parse_args()
    if not args.optimize and not args.compile:
        args.optimize, args.compile = True, True

    names = common.sdfg_names()
    out_label = "default" if args.config is None else args.config
    output_dir = repo_root / common.CODEGEN_DIR / f"stage{STAGE_ID}" / out_label

    if args.optimize:
        for name in names:
            infile = common.stage_input(name, _INPUT_STAGE)
            outfile = output_dir / f"{name}.sdfgz"
            print(f"Stage #{STAGE_ID}: Optimising {name} from {infile}")

            sdfg = dace.SDFG.from_file(infile)
            sdfg.name = name
            sdfg.validate()

            sdfg = optimization_action(sdfg, args.candidates, args.config)

            outfile.parent.mkdir(parents=True, exist_ok=True)
            sdfg.save(str(outfile), compress=True)
            print(f"Stage #{STAGE_ID}: Saved as {outfile.relative_to(repo_root)}")

    if args.compile:
        sdfgs = {
            name: dace.SDFG.from_file(str(output_dir / f"{name}.sdfgz"))
            for name in names
        }
        extra_sources = ["src/reductions.cpp", "src/reductions_kernel.cu"]
        extra_include_dirs = ["include"]
        common.compile_action(
            STAGE_ID, sdfgs,
            gpu=True, release=args.release,
            output=f"velocity_stage{STAGE_ID}_{out_label}",
            extra_sources=extra_sources,
            extra_include_dirs=extra_include_dirs,
        )


if __name__ == "__main__":
    main()
