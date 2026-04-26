"""Regenerate ``include/velocity_tendencies_no_nproma.h`` from an SDFG.

Walks every ``dace.data.Structure`` reachable from the top-level arrays of
``baseline/velocity_no_nproma.sdfgz`` and emits one C++ ``struct`` per
unique Fortran derived type (``Structure.name``). The output is the
header that ``main.cpp`` and ``include/serde_velocity_no_nproma.h``
``#include`` to materialise the call-site argument structs.

By default the script applies the same stripping rules that
``utils/passes/rename_stripped_symbols.RenameStrippedSymbols`` runs on
the SDFG side -- ``__f2dace_<KIND>_`` -> ``<KIND>_`` and ``_s_<NNN>`` ->
"" -- so the regenerated header uses names that are stable across f2dace
runs (the ``_s_<NNN>`` counter is not deterministic). Pass
``--no-strip`` to keep the raw f2dace names if you specifically need to
match an older pinned baseline.

Usage:
    python tools/gen_struct_headers.py
    python tools/gen_struct_headers.py --baseline baseline/X.sdfgz
    python tools/gen_struct_headers.py --output include/Y.h
    python tools/gen_struct_headers.py --no-strip   # raw f2dace names
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple

import dace
from dace import data as dt


# Mirror the strip rules of ``utils.passes.rename_stripped_symbols``. We
# duplicate them here (instead of importing) so the script also works
# from a plain ``python tools/gen_struct_headers.py`` invocation that
# hasn't set ``PYTHONPATH`` to the experiment root.
_F2DACE_SEG = re.compile(r"__f2dace_([A-Za-z][A-Za-z0-9]*)_")
_S_SEG = re.compile(r"_s_\d+")
_TSS_PREFIX = "tmp_struct_symbol_"


def _strip(name: str) -> str:
    new = _F2DACE_SEG.sub(lambda m: f"{m.group(1)}_", name)
    new = _S_SEG.sub("", new)
    if new.startswith(_TSS_PREFIX):
        new = "TSS_" + new[len(_TSS_PREFIX):]
    return new


# ---------- C++ type lowering -------------------------------------------

# DaCe ships ``dtype.ctype`` for every numeric type which is exactly the
# C identifier we want to emit. For Structure members we either emit
# ``<typename>* <field>`` (one indirection level, matches f2dace's
# Fortran allocatable layout) or, for scalar primitives, ``<ctype>
# <field>``.
def _ctype_of(d: dt.Data) -> str:
    """Return the C++ type literal for a Data descriptor."""
    if isinstance(d, dt.Structure):
        return f"{d.name}*"
    if isinstance(d, dt.Scalar):
        return d.dtype.ctype
    if isinstance(d, dt.Array):
        return f"{d.dtype.ctype}*"
    if isinstance(d, dt.View):  # StructureView shouldn't appear inside members
        return f"/* view: {type(d).__name__} */ void*"
    return f"/* unknown: {type(d).__name__} */ void*"


def _is_scalar_member(d: dt.Data) -> bool:
    return isinstance(d, dt.Scalar)


# ---------- Walk + collect ----------------------------------------------

def _collect_structs(sdfg: dace.SDFG) -> Dict[str, dt.Structure]:
    """Return ``{type_name: Structure}`` for every Structure type
    reachable from a top-level non-View Structure descriptor."""
    out: Dict[str, dt.Structure] = {}

    def visit(s: dt.Structure):
        tn = s.name
        if tn in out:
            return
        out[tn] = s
        for _, m in s.members.items():
            if isinstance(m, dt.Structure):
                visit(m)

    for _, s in sdfg.arrays.items():
        # Only proper Structure descriptors define a struct in C++; the
        # ``StructureView`` aliases reuse an existing type.
        if isinstance(s, dt.Structure) and type(s).__name__ != "StructureView":
            visit(s)
    return out


def _topo_order(structs: Dict[str, dt.Structure]) -> List[str]:
    """Return type-names in dependency-first order: a struct's nested
    types come before the struct that contains them."""
    deps: Dict[str, Set[str]] = {tn: set() for tn in structs}
    for tn, s in structs.items():
        for _, m in s.members.items():
            if isinstance(m, dt.Structure) and m.name in structs and m.name != tn:
                deps[tn].add(m.name)

    out: List[str] = []
    seen: Set[str] = set()

    def emit(tn: str, stack: Set[str]):
        if tn in seen:
            return
        if tn in stack:
            # Cycle -- f2dace shouldn't emit these for Fortran derived
            # types, but if it does we break the cycle and rely on
            # forward declarations rather than crashing.
            return
        stack.add(tn)
        for d in sorted(deps[tn]):
            emit(d, stack)
        stack.remove(tn)
        seen.add(tn)
        out.append(tn)

    for tn in sorted(structs):
        emit(tn, set())
    return out


# ---------- Emission ----------------------------------------------------

def _emit_struct(
    name: str,
    s: dt.Structure,
    *,
    strip: bool,
    rename_log: Dict[str, str],
) -> str:
    """Emit a single ``struct <name> { ... };`` block.

    Members are ordered: scalar fields first (SA / SOA / bare ints and
    bare doubles, in iteration order to preserve f2dace's grouping),
    then arrays / nested-struct pointers in iteration order.
    """
    scalar_lines: List[str] = []
    pointer_lines: List[str] = []
    for raw, m in s.members.items():
        new = _strip(raw) if strip else raw
        if strip and new != raw:
            rename_log[raw] = new
        c = _ctype_of(m)
        line = f"    {c} {new} = {{}};"
        (scalar_lines if _is_scalar_member(m) else pointer_lines).append(line)

    body = "\n".join(scalar_lines + pointer_lines) or "    /* empty */"
    return f"struct {name} {{\n{body}\n}};\n"


_HEADER_TEMPLATE = """\
#ifndef __DACE_CODEGEN_VELOCITY_TENDENCIES__
#define __DACE_CODEGEN_VELOCITY_TENDENCIES__

// Auto-generated by tools/gen_struct_headers.py. Do not edit by hand.
//
// Source SDFG: {source}
// strip-mode : {strip_mode}
//
// One C++ struct per Fortran derived type reachable from the top-level
// SDFG containers (p_patch, p_diag, p_int, p_metrics, p_prog,
// global_data, ...). Order is dependency-first: a nested struct type is
// emitted before the parent struct that uses it.

#include <dace/dace.h>


{forward_decls}

{body}
#endif  // __DACE_CODEGEN_VELOCITY_TENDENCIES__
"""


def emit_header(sdfg_path: Path, *, strip: bool) -> Tuple[str, Dict[str, str]]:
    sdfg = dace.SDFG.from_file(str(sdfg_path))
    structs = _collect_structs(sdfg)
    if not structs:
        raise SystemExit(
            f"[gen_struct_headers] no top-level Structure descriptors in "
            f"{sdfg_path}; this script expects the pre-flatten "
            f"baseline (e.g. baseline/velocity_no_nproma.sdfgz)."
        )

    order = _topo_order(structs)
    rename_log: Dict[str, str] = {}
    blocks = [
        _emit_struct(tn, structs[tn], strip=strip, rename_log=rename_log)
        for tn in order
    ]
    fwd = "\n".join(f"struct {tn};" for tn in order)
    body = "\n".join(blocks)
    text = _HEADER_TEMPLATE.format(
        source=sdfg_path,
        strip_mode="stripped (RenameStrippedSymbols rules applied)" if strip else "raw f2dace",
        forward_decls=fwd,
        body=body,
    )
    return text, rename_log


def main(argv: Optional[List[str]] = None):
    argp = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    argp.add_argument(
        "--baseline",
        default="baseline/velocity_no_nproma.sdfgz",
        help="SDFG to read struct hierarchy from (default: pre-flatten baseline).",
    )
    argp.add_argument(
        "--output",
        default="include/velocity_tendencies_no_nproma.h",
        help="Header file to write (default: include/velocity_tendencies_no_nproma.h).",
    )
    argp.add_argument(
        "--no-strip",
        dest="strip",
        action="store_false",
        default=True,
        help=("Keep raw __f2dace_*_s_<NNN> names instead of applying the "
              "RenameStrippedSymbols strip rules (default: strip)."),
    )
    args = argp.parse_args(argv)

    sdfg_path = Path(args.baseline)
    if not sdfg_path.is_file():
        sys.stderr.write(f"[gen_struct_headers] missing input: {sdfg_path}\n")
        sys.exit(1)

    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    text, rename_log = emit_header(sdfg_path, strip=args.strip)
    out_path.write_text(text)
    print(f"[gen_struct_headers] wrote {out_path}  "
          f"({'stripped' if args.strip else 'raw'} names)")
    if args.strip:
        print(f"[gen_struct_headers] renamed {len(rename_log)} member name(s) "
              f"from raw f2dace form")


if __name__ == "__main__":
    main()
