"""Generate the first SDFG from the velocity-tendencies Fortran source.

Pipeline order:
    1. python f90_to_sdfg.py             THIS                  (f2dace/staging)
    2. python generate_baselines.py      4 specialised variants (f2dace/staging)
    3. python -m utils.stages.stage1     freeze the kernel ABI  (yakup/dev)

Reads ``baseline_inputs/velocity_modified.f90`` -- the preprocessed
self-contained Fortran AST (every ``mo_*`` dependency reachable from
``mo_velocity_advection.velocity_tendencies`` is already inlined and
``mo_exception``/``mo_real_timer`` are stubbed) -- and emits
``baseline/velocity_no_nproma.sdfgz``, the input ``generate_baselines.py``
expects.

DaCe must be on a branch shipping the f2dace Fortran frontend. The
script verifies the active checkout matches the pinned commit below;
override with ``--allow-commit-drift`` if you intentionally bumped DaCe.

The pin matters: f2dace's mangled symbol counter (``_s_<NNN>``) and the
order in which ``StructToContainerGroups`` visits nested descriptors are
sensitive to the exact DaCe revision. Without a stable pin the
post-flatten SDFG drifts byte-for-byte across reviewer machines, and the
committed C++ struct headers stop matching the regenerated SDFG.

Usage:
    python f90_to_sdfg.py
    python f90_to_sdfg.py --input X.f90 --output Y.sdfgz
    python f90_to_sdfg.py --allow-commit-drift
"""

from __future__ import annotations

import argparse
import re
import shutil
import sys
import tempfile
from pathlib import Path

from utils.dace_branch import F2DACE_BRANCH, F2DACE_COMMIT, ensure_branch


_ENTRY_POINT = ("mo_velocity_advection", "velocity_tendencies")


def _normalize_logicals(src: str) -> str:
    """f2dace doesn't lower Fortran ``LOGICAL(KIND=1)`` cleanly. The
    icon-artifacts recipe runs ``sed 's/LOGICAL(KIND=1)/INTEGER(KIND=4)/g'``
    between stages; do the same here defensively (no-op on the file we
    ship today, but future AST snapshots may regress)."""
    return re.sub(r"LOGICAL\(KIND\s*=\s*1\)", "INTEGER(KIND=4)", src)


def _import_f2dace():
    try:
        from dace.frontend.fortran.fortran_parser import (
            ParseConfig,
            SDFGConfig,
            create_internal_ast,
            create_sdfg_from_internal_ast,
        )
    except ImportError as e:
        sys.stderr.write(
            "[f90_to_sdfg] cannot import f2dace fortran_parser. Make sure "
            f"DaCe is on {F2DACE_BRANCH} (or any branch shipping the "
            "Fortran frontend) and visible to this Python.\n"
            f"  ImportError: {e}\n"
        )
        sys.exit(2)
    return ParseConfig, SDFGConfig, create_internal_ast, create_sdfg_from_internal_ast


def main(argv=None):
    argp = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    argp.add_argument(
        "--input",
        default="baseline_inputs/velocity_modified.f90",
        help="Preprocessed velocity AST in Fortran "
             "(default: baseline_inputs/velocity_modified.f90).",
    )
    argp.add_argument(
        "--output",
        default="baseline/velocity_no_nproma.sdfgz",
        help="SDFG to write (default: baseline/velocity_no_nproma.sdfgz).",
    )
    argp.add_argument(
        "--no-simplify",
        action="store_true",
        help="Skip ``sdfg.simplify()`` after f2dace returns.",
    )
    argp.add_argument(
        "--keep-tmp",
        action="store_true",
        help="Keep the post-sed normalised .f90 in the temp dir for debugging.",
    )
    argp.add_argument(
        "--allow-commit-drift",
        action="store_true",
        help=("Proceed even if the active DaCe checkout is not on the "
              f"pinned commit {F2DACE_COMMIT[:12]} ({F2DACE_BRANCH})."),
    )
    args = argp.parse_args(argv)

    ensure_branch(F2DACE_BRANCH, commit=F2DACE_COMMIT,
                  allow_drift=args.allow_commit_drift)

    src_path = Path(args.input).resolve()
    if not src_path.is_file():
        sys.stderr.write(f"[f90_to_sdfg] missing input: {src_path}\n")
        sys.exit(1)

    out_path = Path(args.output).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)

    src = src_path.read_text()
    norm = _normalize_logicals(src)
    if norm != src:
        print("[f90_to_sdfg] LOGICAL(KIND=1) -> INTEGER(KIND=4) substitutions applied")

    tmpdir = Path(tempfile.mkdtemp(prefix="velocity_f2dace_"))
    norm_path = tmpdir / "velocity_modified_norm.f90"
    norm_path.write_text(norm)

    print(f"[f90_to_sdfg] input  : {src_path}")
    print(f"[f90_to_sdfg] entry  : {'.'.join(_ENTRY_POINT)}")
    print(f"[f90_to_sdfg] output : {out_path}")

    ParseConfig, SDFGConfig, create_internal_ast, create_sdfg_from_internal_ast = _import_f2dace()

    cfg = ParseConfig(sources=[norm_path], entry_points=[_ENTRY_POINT])
    own_ast, program = create_internal_ast(cfg)

    sdfg_cfg = SDFGConfig(
        {_ENTRY_POINT[-1]: _ENTRY_POINT},
        normalize_offsets=True,
        multiple_sdfgs=False,
    )
    gmap = create_sdfg_from_internal_ast(own_ast, program, sdfg_cfg)
    if _ENTRY_POINT[-1] not in gmap:
        sys.stderr.write(
            f"[f90_to_sdfg] f2dace returned {list(gmap.keys())}; "
            f"expected key '{_ENTRY_POINT[-1]}'\n"
        )
        sys.exit(3)

    sdfg = gmap[_ENTRY_POINT[-1]]
    sdfg.name = out_path.stem

    sdfg.save(str(out_path), compress=str(out_path).endswith(".sdfgz"))
    if not args.no_simplify:
        sdfg.simplify()
        sdfg.save(str(out_path), compress=str(out_path).endswith(".sdfgz"))

    if not args.keep_tmp:
        shutil.rmtree(tmpdir, ignore_errors=True)

    print(f"[f90_to_sdfg] wrote SDFG to {out_path}")


if __name__ == "__main__":
    main()
