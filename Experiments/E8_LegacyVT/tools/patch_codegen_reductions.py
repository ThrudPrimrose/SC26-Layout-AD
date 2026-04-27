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
import os
import re
import sys
from pathlib import Path
from typing import Iterable, List, Tuple

REDUCTIONS_INCLUDE = '#include "reductions_kernel.cuh"'
REDUCTIONS_CPU_INCLUDE = '#include "reductions_cpu.h"'
# ``reductions_device.cuh`` defines ``__REDUCE_DEVICE__`` and the
# ``__device__ __inline__ reduce_*_device(...)`` family. Including it
# in any pure-device codegen file flips the tasklets' #ifdef chain so
# the ``__REDUCE_DEVICE__`` branch wins (instead of ``__REDUCE_GPU__``,
# which calls the host-side ``reduce_*_gpu`` and therefore fails to
# compile from a ``__device__`` scope).
REDUCTIONS_DEVICE_INCLUDE = '#include "reductions_device.cuh"'
DACE_INCLUDE_RE = re.compile(r'^#include\s*<dace/dace\.h>\s*$', re.MULTILINE)

# AMD detection: same env signals as compile_if_propagated_sdfgs.py.
_AMD = (
    os.getenv("__HIP_PLATFORM_AMD__", "0") == "1"
    or os.getenv("HIP_PLATFORM_AMD", "0") == "1"
    or os.getenv("BEVERIN", "0") == "1"
)

# DaCe's CPU codegen target hardcodes <cuda_runtime.h> in the host-side
# glue (src/cpu/*.cpp) even when ``compiler.cuda.backend = "hip"`` is
# set. ROCm clang++ then chokes with ``cuda_runtime.h: file not found``.
# Rewrite to the HIP header. Idempotent (skips files that already use
# hip/hip_runtime.h). AMD-only -- on NVIDIA we leave the include alone.
_CUDA2HIP_HEADERS = [
    (re.compile(r'#include\s*<cuda_runtime\.h>'),     '#include <hip/hip_runtime.h>'),
    (re.compile(r'#include\s*<cuda_runtime_api\.h>'), '#include <hip/hip_runtime_api.h>'),
]

# Match every USE of ``__dace_current_stream`` as a value (function
# argument, return expression, comparison RHS), but NOT:
#   * the typed declaration ``cudaStream_t __dace_current_stream = ...``
#     -- ``(?<!_t\s)`` negative lookbehind
#   * the LHS of any assignment ``__dace_current_stream = ...``
#     -- ``(?!\s*=[^=])`` negative lookahead (allows ``==`` comparisons)
#   * the adjacent ``__dace_current_stream_id`` integer
#     -- ``\b...\b`` word boundaries reject the longer identifier
# Replacement is global: when DaCe didn't emit the prelude in the
# enclosing scope (the cause of the ``undeclared identifier`` errors
# on permuted SDFGs), every value-use falls back to the default-stream
# nullptr.
_STREAM_REF_RE = re.compile(
    r'(?<!_t\s)\b__dace_current_stream\b(?!\s*=[^=])'
)


def _patch_one(path: Path) -> Tuple[bool, bool]:
    """Patch a pure-device codegen file (``*_cuda.cu`` or
    ``src/cuda/hip/*_cuda.cpp``).

    Two injections + one rewrite:
      1. ``#include "reductions_kernel.cuh"`` -- declares the host-side
         ``reduce_*_gpu(stream)`` family. Kept for completeness; the
         dominant code path no longer needs it once the tasklet flips
         to the device branch (see #2).
      2. ``#include "reductions_device.cuh"`` -- defines
         ``__REDUCE_DEVICE__`` AND the ``__device__ reduce_*_device``
         functions. With ``__REDUCE_DEVICE__`` defined, the tasklet's
         ``#ifdef __REDUCE_DEVICE__`` branch wins ahead of
         ``__REDUCE_GPU__``, calling the device-callable variant. This
         is required because the tasklet bodies are inlined into
         ``__device__ loop_body_*`` helpers by the permute pass, and
         calling the host ``reduce_scan_gpu`` from device scope is a
         hard nvcc error.
      3. All value-uses of ``__dace_current_stream`` -> ``nullptr``
         (declarations + LHS-of-assignment kept intact).
    """
    text = path.read_text()
    added_include = False
    replaced_stream = False

    if "reduce_" in text:
        match = DACE_INCLUDE_RE.search(text)
        injections = []
        if REDUCTIONS_INCLUDE not in text:
            injections.append(REDUCTIONS_INCLUDE)
        if REDUCTIONS_DEVICE_INCLUDE not in text:
            injections.append(REDUCTIONS_DEVICE_INCLUDE)
        if injections:
            block = "\n" + "\n".join(injections)
            if match:
                insert_at = match.end()
                text = text[:insert_at] + block + text[insert_at:]
            else:
                text = block.lstrip("\n") + "\n" + text
            added_include = True

    new_text, n = _STREAM_REF_RE.subn('nullptr', text)
    if n > 0:
        text = new_text
        replaced_stream = True

    if added_include or replaced_stream:
        path.write_text(text)
    return added_include, replaced_stream


def _patch_cpu_includes(path: Path) -> bool:
    """AMD-only: rewrite ``<cuda_runtime.h>`` -> ``<hip/hip_runtime.h>``
    (and the _api variant) in a CPU-glue ``.cpp`` file. Returns True iff
    the file was modified. Idempotent.
    """
    text = path.read_text()
    new_text = text
    for pat, repl in _CUDA2HIP_HEADERS:
        new_text = pat.sub(repl, new_text)
    if new_text != text:
        path.write_text(new_text)
        return True
    return False


def _patch_cpu_reductions(path: Path) -> Tuple[bool, bool]:
    """Inject ``reductions_kernel.cuh`` + ``reductions_cpu.h`` and
    rewrite ``__dace_current_stream`` -> ``nullptr`` in CPU ``.cpp``
    files generated by stage 8. Same failure mode as the .cu patch,
    but the host-side glue calls ``reduce_maxZ_to_address_gpu``,
    ``reduce_scan_last_dim``, ``reduce_maxZ_to_scalar_cpu`` etc.
    without including either header. Platform-agnostic (runs on both
    NVIDIA and AMD). Returns ``(added_includes, replaced_stream)``.
    Idempotent.
    """
    text = path.read_text()
    added_includes = False
    replaced_stream = False

    needs_kernel = REDUCTIONS_INCLUDE not in text
    needs_cpu    = REDUCTIONS_CPU_INCLUDE not in text
    has_calls    = "reduce_" in text
    if has_calls and (needs_kernel or needs_cpu):
        block = ""
        if needs_kernel: block += "\n" + REDUCTIONS_INCLUDE
        if needs_cpu:    block += "\n" + REDUCTIONS_CPU_INCLUDE
        match = DACE_INCLUDE_RE.search(text)
        if match:
            insert_at = match.end()
            text = text[:insert_at] + block + text[insert_at:]
        else:
            text = block.lstrip("\n") + "\n" + text
        added_includes = True

    new_text, n = _STREAM_REF_RE.subn('nullptr', text)
    if n > 0:
        text = new_text
        replaced_stream = True

    if added_includes or replaced_stream:
        path.write_text(text)
    return added_includes, replaced_stream


def patch_codegen_tree(roots: Iterable[Path], verbose: bool = True) -> int:
    """Walk *roots* (one or more codegen / SDFG build_folder paths) and
    apply post-codegen patches:
      * every ``src/cuda/*.cu``: inject reductions_kernel.cuh,
        rewrite ``__dace_current_stream`` -> nullptr;
      * every ``src/cpu/*.cpp``: same reductions injection (also pulls
        in reductions_cpu.h for the CPU ``reduce_*_cpu`` family) +
        nullstream rewrite (platform-agnostic);
      * AMD only: cuda_runtime.h -> hip/hip_runtime.h in CPU ``.cpp``.
    Returns total file count patched. Programmatic entry point for
    ``post_codegen_hook``.
    """
    n_patched = 0
    for root in roots:
        root = Path(root)
        if not root.exists():
            continue
        # Device-side codegen lives at:
        #   * NVIDIA:  src/cuda/<name>_cuda.cu
        #   * AMD/HIP: src/cuda/hip/<name>_cuda.cpp
        # Walk both (and any future variants) by globbing src/cuda/
        # recursively for .cu OR .cpp.
        device_files = sorted(
            list(root.rglob("src/cuda/*.cu"))
            + list(root.rglob("src/cuda/**/*.cu"))
            + list(root.rglob("src/cuda/**/*.cpp"))
        )
        # de-dup while preserving order
        seen: set = set()
        device_files = [p for p in device_files
                        if not (p in seen or seen.add(p))]
        for f in device_files:
            inc, sub = _patch_one(f)
            if inc or sub:
                n_patched += 1
                if verbose:
                    tags = []
                    if inc: tags.append("+include")
                    if sub: tags.append("+nullstream")
                    print(f"  [patch_codegen_reductions] {f}  [{','.join(tags)}]")
        for cpp in sorted(root.rglob("src/cpu/*.cpp")):
            inc, sub = _patch_cpu_reductions(cpp)
            hip_rewrite = _AMD and _patch_cpu_includes(cpp)
            if inc or sub or hip_rewrite:
                n_patched += 1
                if verbose:
                    tags = []
                    if inc:        tags.append("+includes")
                    if sub:        tags.append("+nullstream")
                    if hip_rewrite: tags.append("+hip_runtime")
                    print(f"  [patch_codegen_reductions] {cpp}  [{','.join(tags)}]")
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

    # Match both NVIDIA (.cu) and AMD (.cpp under src/cuda/hip/).
    cu_files = sorted(set(
        list(args.codegen_root.rglob("src/cuda/*.cu"))
        + list(args.codegen_root.rglob("src/cuda/**/*.cu"))
        + list(args.codegen_root.rglob("src/cuda/**/*.cpp"))
    ))
    if not cu_files:
        print(f"[patch_codegen_reductions] no device source files under "
              f"{args.codegen_root}/src/cuda/; nothing to do.")
        return 0

    n_inc, n_str, n_files = 0, 0, 0
    for p in cu_files:
        if args.dry_run:
            text = p.read_text()
            needs_inc = REDUCTIONS_INCLUDE not in text and "reduce_" in text
            needs_str = bool(_STREAM_REF_RE.search(text))
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
