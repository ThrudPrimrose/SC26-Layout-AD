#!/usr/bin/env python3
"""
patch_codegen_reductions.py -- post-codegen fixup for stage-8 permuted
SDFGs whose tasklets call ``reduce_scan_gpu`` / ``reduce_sum_to_scalar_gpu``
/ ``reduce_maxZ_to_scalar_gpu`` from the E8 ``include/reductions_kernel.cuh``
shim.

Two issues this patches:

1. **Missing include.** DaCe's CUDA codegen emits only
   ``#include <cuda_runtime.h>`` + ``#include <dace/dace.h>`` at the top
   of each generated ``*_cuda.cu``. The reduction tasklets call
   ``reduce_scan_gpu(...)`` without including the header that declares
   it -- breaking nvcc with ``identifier "reduce_scan_gpu" is undefined``.

2. **Out-of-scope stream variable.** The tasklets reference
   ``__dace_current_stream``, which DaCe emits as a CPU-prelude local
   only when the tasklet has ``_cuda_stream`` set AND lives at host
   scope. The stage-8 permute pass inlines reduction tasklets into
   ``loop_body_*`` helpers, where the prelude variable isn't visible.
   Replace with ``nullptr`` -- the upstream reductions_kernel impl
   accepts a null stream.

Idempotent: safe to re-run; lines that already have the include or
already use ``nullptr`` are left alone.

Usage:
    python tools/patch_codegen_reductions.py [<codegen_root>]

    # default: ./codegen/
    # invoked from run_*.sh after compile_gpu_stage8 codegen and before nvcc.
"""

import argparse
import re
import sys
from pathlib import Path
from typing import Iterable, List, Tuple

REDUCTIONS_INCLUDE = '#include "reductions_kernel.cuh"'
DACE_INCLUDE_RE = re.compile(r'^#include\s*<dace/dace\.h>\s*$', re.MULTILINE)

# Match calls like ``reduce_scan_gpu(in_arr, in_size, __dace_current_stream)``
# anywhere in the file. Conservative: only rewrite the third argument and
# keep everything else as-is. The trailing ``)`` is preserved by the
# capture groups.
_STREAM_CALL_RE = re.compile(
    r'(reduce_(?:scan|sum_to_scalar|maxZ_to_scalar|sum_to_address|maxZ_to_address)_gpu\s*\([^)]*?,\s*)__dace_current_stream(\s*\))'
)


def _patch_one(path: Path) -> Tuple[bool, bool]:
    """Returns (added_include, replaced_stream)."""
    text = path.read_text()
    added_include = False
    replaced_stream = False

    if REDUCTIONS_INCLUDE not in text and "reduce_" in text:
        # Inject right after ``#include <dace/dace.h>`` -- if that line
        # isn't there for some reason, fall back to top of file.
        match = DACE_INCLUDE_RE.search(text)
        if match:
            insert_at = match.end()
            text = text[:insert_at] + "\n" + REDUCTIONS_INCLUDE + text[insert_at:]
        else:
            text = REDUCTIONS_INCLUDE + "\n" + text
        added_include = True

    new_text, n = _STREAM_CALL_RE.subn(r'\1nullptr\2', text)
    if n > 0:
        text = new_text
        replaced_stream = True

    if added_include or replaced_stream:
        path.write_text(text)
    return added_include, replaced_stream


def patch_codegen_tree(roots: Iterable[Path], verbose: bool = True) -> int:
    """Walk *roots* (one or more codegen / SDFG build_folder paths),
    patch every ``src/cuda/*.cu`` under each. Returns total file count
    patched. Programmatic entry point for ``post_codegen_hook``.
    """
    n_patched = 0
    for root in roots:
        root = Path(root)
        if not root.exists():
            continue
        for cu in sorted(root.rglob("src/cuda/*.cu")):
            inc, sub = _patch_one(cu)
            if inc or sub:
                n_patched += 1
                if verbose:
                    tags = []
                    if inc: tags.append("+include")
                    if sub: tags.append("+nullstream")
                    print(f"  [patch_codegen_reductions] {cu}  [{','.join(tags)}]")
    return n_patched


def post_codegen_hook(sdfgs) -> None:
    """``compile_if_propagated_sdfgs`` post-codegen hook signature.

    Walks every SDFG's ``build_folder`` and patches its ``src/cuda/*.cu``
    files in place. Idempotent. Safe to attach to any compile_action
    invocation -- if no .cu files have reduction tasklets, the patcher
    is a no-op.
    """
    roots = []
    for s in sdfgs:
        bf = getattr(s, "build_folder", None)
        if bf and bf not in (".", ""):
            roots.append(Path(bf))
    n = patch_codegen_tree(roots, verbose=True)
    print(f"[patch_codegen_reductions] post_codegen_hook: patched {n} files "
          f"across {len(roots)} SDFG build folders")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("codegen_root", nargs="?", type=Path, default=Path("codegen"),
                    help="codegen tree root (default: ./codegen)")
    ap.add_argument("--dry-run", action="store_true",
                    help="report which files would be patched without writing")
    args = ap.parse_args()

    if not args.codegen_root.exists():
        print(f"[patch_codegen_reductions] {args.codegen_root}/ does not exist; "
              f"nothing to do.", file=sys.stderr)
        return 0

    cu_files = sorted(args.codegen_root.rglob("src/cuda/*.cu"))
    if not cu_files:
        print(f"[patch_codegen_reductions] no .cu files under "
              f"{args.codegen_root}/; nothing to do.")
        return 0

    n_inc, n_str, n_files = 0, 0, 0
    for p in cu_files:
        if args.dry_run:
            text = p.read_text()
            needs_inc = REDUCTIONS_INCLUDE not in text and "reduce_" in text
            needs_str = bool(_STREAM_CALL_RE.search(text))
            if needs_inc or needs_str:
                tags = []
                if needs_inc: tags.append("+include")
                if needs_str: tags.append("+nullstream")
                print(f"  {p}  [{','.join(tags)}]")
                n_files += 1
        else:
            inc, sub = _patch_one(p)
            if inc or sub:
                tags = []
                if inc: tags.append("+include")
                if sub: tags.append("+nullstream")
                print(f"  {p}  [{','.join(tags)}]")
                n_inc += int(inc)
                n_str += int(sub)
                n_files += 1

    if args.dry_run:
        print(f"\n[dry-run] {n_files} files would be patched.")
    else:
        print(f"\n[patch_codegen_reductions] patched {n_files} files "
              f"(+include: {n_inc}, +nullstream: {n_str}).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
