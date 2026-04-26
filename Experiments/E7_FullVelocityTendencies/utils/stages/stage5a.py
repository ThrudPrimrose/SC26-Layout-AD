"""Stage 5a: GPU layout-permutation sweep (alternative to stage 5b).

Reads ``codegen/stage4/<name>.sdfgz`` and, for each requested
``PermuteConfig``, deepcopies the SDFG, applies the permutation, and
saves ``codegen/stage5a/<config>/<name>.sdfgz``. Compile mode
(``--compile``) runs a per-config codegen + build, producing
``velocity_stage5a_<config>`` binaries.

Stage 5a and stage 5b are mutually exclusive paths off stage 4: 5a
adds permutations, 5b adds neighborhood-list / block-array
compression (plus an always-on ``levmask`` transpose). Pick whichever
matches the experiment.

GPU-only: the permutation transform is built around core-dialect
semantics and only the post-stage-4 GPU SDFGs satisfy it. CPU SDFGs
(top-level map schedule != GPU_Device) are skipped with a notice.

Usage::

    python -m utils.stages.stage5a                       # default configs, optimize+compile
    python -m utils.stages.stage5a --optimize
    python -m utils.stages.stage5a --compile
    python -m utils.stages.stage5a --config nlev_first   # single named config
"""

import argparse
import copy
from pathlib import Path
from typing import Dict, List

from utils.dace_branch import YAKUP_DEV_BRANCH, ensure_branch
ensure_branch(YAKUP_DEV_BRANCH)

import dace

from utils.passes.permute_configs import extended_configs_from_candidates
from utils.passes.permute_layout import PermuteConfig, permute_layout
from utils.stages import common


STAGE_ID = "5a"
# stage 5a reads from stage 4's output, regardless of the string id.
_INPUT_STAGE = 5  # ``stage_input(name, 5)`` -> ``codegen/stage4/<name>.sdfgz``


def _sdfg_uses_gpu(sdfg: dace.SDFG) -> bool:
    for node, _ in sdfg.all_nodes_recursive():
        if isinstance(node, dace.nodes.MapEntry):
            if node.map.schedule == dace.ScheduleType.GPU_Device:
                return True
    return False


def _array_dim_map(sdfg: dace.SDFG) -> Dict[str, int]:
    return {name: len(arr.shape) for name, arr in sdfg.arrays.items()}


def _select_configs(name_filter: str | None, json_path: Path,
                    sdfg_arrays: Dict[str, int] | None) -> List[PermuteConfig]:
    configs = extended_configs_from_candidates(json_path, sdfg_arrays=sdfg_arrays)
    if name_filter is None:
        return configs
    selected = [c for c in configs if c.name == name_filter]
    if not selected:
        known = ", ".join(c.name for c in configs)
        raise SystemExit(f"Stage #{STAGE_ID}: no config named '{name_filter}' (known: {known})")
    return selected


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
                      help="run only the named config (default: every config from JSON)")
    argp.add_argument("--candidates", type=Path, default=default_json,
                      help="path to layout_candidates.json")
    mode = argp.add_mutually_exclusive_group()
    mode.add_argument("--release", dest="release", action="store_true",
                      help="build with -O3 + --use_fast_math (FMA may diverge from IEEE)")
    mode.add_argument("--debug", dest="release", action="store_false",
                      help="build with -O0 + DACE_VELOCITY_DEBUG + IEEE FP (default)")
    argp.set_defaults(release=False)
    args = argp.parse_args()
    if not args.optimize and not args.compile:
        args.optimize = args.compile = True

    names = common.sdfg_names()

    if args.optimize:
        for name in names:
            infile = common.stage_input(name, _INPUT_STAGE)
            print(f"Stage #{STAGE_ID}: loading {name} from {infile}")
            sdfg = dace.SDFG.from_file(infile)
            sdfg.name = name

            if not _sdfg_uses_gpu(sdfg):
                print(f"  [skip] {name}: no GPU_Device map; permutations are GPU-only")
                continue

            configs = _select_configs(args.config, args.candidates, _array_dim_map(sdfg))
            for cfg in configs:
                permuted = copy.deepcopy(sdfg)
                permuted.name = name
                count = permute_layout(permuted, cfg, shuffle_map=True)
                print(f"  [{cfg.name}] permuted {count} array(s)")

                outfile = (repo_root / common.CODEGEN_DIR / f"stage{STAGE_ID}"
                           / cfg.name / f"{name}.sdfgz")
                outfile.parent.mkdir(parents=True, exist_ok=True)
                permuted.save(str(outfile), compress=True)
                print(f"  [{cfg.name}] saved {outfile.relative_to(repo_root)}")

    if args.compile:
        configs = _select_configs(args.config, args.candidates, sdfg_arrays=None)
        for cfg in configs:
            sdfgs: Dict[str, dace.SDFG] = {}
            for name in names:
                stage_path = (repo_root / common.CODEGEN_DIR / f"stage{STAGE_ID}"
                              / cfg.name / f"{name}.sdfgz")
                if not stage_path.exists():
                    print(f"  [skip-compile] {cfg.name}/{name}: no SDFG (run --optimize first)")
                    continue
                sdfgs[name] = dace.SDFG.from_file(str(stage_path))
            if not sdfgs:
                continue
            common.compile_action(
                STAGE_ID,
                sdfgs,
                gpu=True,
                release=args.release,
                output=f"velocity_stage{STAGE_ID}_{cfg.name}",
                extra_sources=["src/reductions.cpp", "src/reductions_kernel.cu"],
                extra_include_dirs=["include"],
            )


if __name__ == "__main__":
    main()
