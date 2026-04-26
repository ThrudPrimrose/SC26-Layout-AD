"""Regenerate ``include/call_velocity.h`` from the frozen stage1 SDFG.

Pairs with ``tools/gen_struct_headers.py`` -- both run the same name-strip
rules (``__f2dace_<KIND>_`` -> ``<KIND>_``, drop ``_s_<NNN>``,
``tmp_struct_symbol_`` -> ``TSS_``), so the C++ struct definitions and
the macro arglist agree on every member name.

Emits two macros consumed by ``main.cpp``:

  - ``VELOCITY_CALL_ARGS``: positional args for ``__program_*`` -- struct
    field references that match each non-transient array kernel param.
  - ``VELOCITY_INIT_ARGS``: positional args for ``__dace_init_*`` -- struct
    field references that match each int kernel-init shape extent.

Param names produced by stage1 are already stripped; this script just
needs to derive the C++ field-access path from the name shape:

  - ``__CG_<a>__CG_<b>__m_<x>``   -> ``a.b->x``           (multi-level)
  - ``__CG_<container>__m_<x>``    -> ``container.x``     (single level)
  - ``<KIND>_<rest>_<container>_<num>`` -> ``container.<inner>-><KIND>_<head>``
                                                          (nested struct extent)
  - ``<KIND>_<arr>_d_<dim>``      -> ``serde::ARRAY_META_DICT_AT(<arr>)
                                       .{size,lbound}.at(<dim>)``
  - ``TSS_<N>``                    -> rewrite of init_code RHS for that N
  - bare names (``nproma`` / ``ntnd`` / ``dtime`` / ...) -> caller-supplied
    or struct-known scalar; matched against the committed bare-member set.

Usage:
    python tools/gen_call_site.py
    python tools/gen_call_site.py --baseline codegen/stage1/<name>.sdfgz
    python tools/gen_call_site.py --output include/call_velocity.h
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import dace
from dace import data as dt


# ---------- Top-level container layout ----------------------------------

# Top-level struct containers main.cpp passes by name. Order matters only
# for fallback emission; the actual macro arg order comes from the SDFG
# arglist.
_TOP_STRUCTS = (
    "global_data",
    "p_diag",
    "p_int",
    "p_metrics",
    "p_patch",
    "p_prog",
)

# Top-level array names main.cpp passes by name (no enclosing struct).
# Reach their size / lbound through ``serde::ARRAY_META_DICT_AT``.
_TOP_ARRAYS = (
    "z_kin_hor_e",
    "z_vt_ie",
    "z_w_concorr_me",
    "dt_linintp_ubc",
)


# ---------- Strip helpers (mirror RenameStrippedSymbols) ----------------

_F2DACE_SEG = re.compile(r"__f2dace_([A-Za-z][A-Za-z0-9]*)_")
_S_SEG = re.compile(r"_s_\d+")
_TSS_PREFIX = "tmp_struct_symbol_"


def _strip(name: str) -> str:
    new = _F2DACE_SEG.sub(lambda m: f"{m.group(1)}_", name)
    new = _S_SEG.sub("", new)
    if new.startswith(_TSS_PREFIX):
        new = "TSS_" + new[len(_TSS_PREFIX):]
    return new


# ---------- Init code lookup for TSS_<N> --------------------------------

_TSS_RE = re.compile(r"^TSS_(\d+)$")
# Both unstripped and stripped LHS forms appear depending on which SDFG
# we read from -- accept either.
_TSS_INIT_RE = re.compile(
    r"^\s*(?:tmp_struct_symbol|TSS)_(\d+)\s*=\s*(.+?)\s*;\s*$"
)


def _tss_map(unstripped_sdfg_path: Path) -> Dict[int, str]:
    """Return ``{N: cpp_rhs}`` for every ``tmp_struct_symbol_<N>=...;`` /
    ``TSS_<N>=...;`` line found in the SDFG's ``init_code['frame']``.

    The RHS is rewritten so top-level ``container->member`` becomes
    ``container.member`` (main.cpp passes top-level containers by value
    or reference, not pointer). Nested ``->`` chains are preserved (the
    nested members remain pointer fields). The RHS is also passed
    through ``_strip`` so any residual ``__f2dace_*_s_NNN`` references
    -- if reading the unstripped baseline -- become stripped names that
    match the regenerated C++ struct headers.
    """
    sdfg = dace.SDFG.from_file(str(unstripped_sdfg_path))
    out: Dict[int, str] = {}
    for tgt, code in sdfg.init_code.items():
        if hasattr(code, "code") and isinstance(code.code, str):
            txt = code.code
        elif hasattr(code, "as_string"):
            txt = code.as_string
        else:
            txt = str(code)
        for line in txt.splitlines():
            m = _TSS_INIT_RE.match(line)
            if not m:
                continue
            idx = int(m.group(1))
            rhs = _strip(m.group(2))
            rhs = rhs.replace("global_data->", "global_data.")
            for cn in _TOP_STRUCTS:
                rhs = rhs.replace(f"{cn}->", f"{cn}.")
            out[idx] = rhs
    return out


# ---------- Bare struct members (nlev / id / nblks_c / ...) -------------

def _bare_members(unstripped_sdfg_path: Path) -> Dict[str, str]:
    """Return ``{member_name: container_name}`` for every bare struct
    member (no f2dace prefix) found at depth 1 of the unstripped
    baseline. Lets us route lone names like ``nlev``, ``id``, ``nproma``
    to the right ``<container>.<member>``.

    On collision (same bare name in two top-level structs), prefer the
    first ``_TOP_STRUCTS``-order match.
    """
    sdfg = dace.SDFG.from_file(str(unstripped_sdfg_path))
    out: Dict[str, str] = {}
    for cn in _TOP_STRUCTS:
        s = sdfg.arrays.get(cn)
        if not isinstance(s, dt.Structure):
            continue
        for raw, _ in s.members.items():
            if "__f2dace" in raw or "_s_" in raw:
                continue
            stripped = _strip(raw)
            out.setdefault(stripped, cn)
    return out


# ---------- Param-name -> C++ source mapping ----------------------------

_NESTED_RE = re.compile(
    r"^(SA|SOA|A|OA|S)_(.+)_(p_patch|p_int|p_diag|p_metrics|p_prog|global_data)_(\d+)$"
)
_TOP_AOA_RE = re.compile(r"^(A|OA)_([A-Za-z_]\w*?)_d_(\d+)$")


def _split_cg(name: str) -> Optional[Tuple[List[str], str]]:
    """Parse ``__CG_a__CG_b__...__m_x`` into ``([a, b, ...], x)``.

    The kernel arglist uses this shape for both single-level
    (``__CG_p_diag__m_max_vcfl_dyn``) and multi-level
    (``__CG_p_patch__CG_cells__CG_decomp_info__m_owner_mask``)
    container chains.
    """
    if not name.startswith("__CG_"):
        return None
    rest = name[len("__CG_"):]
    segs = rest.split("__CG_")
    last = segs[-1]
    if "__m_" not in last:
        return None
    last_container, member = last.split("__m_", 1)
    return ([*segs[:-1], last_container], member)


def _scalar_struct_members(unstripped_sdfg_path: Path) -> set:
    """Return ``{<container>.<stripped_member>}`` for every Scalar member
    of a top-level struct in the unstripped baseline.

    Used to recognise call-site args that the kernel takes by pointer
    (e.g. ``promote_maxvcfl`` rewrites ``max_vcfl_dyn`` from a scalar
    output to a length-1 Array) but the C++ struct still holds as a
    value field. Those need ``&<container>.<member>`` to bridge.
    """
    sdfg = dace.SDFG.from_file(str(unstripped_sdfg_path))
    out: set = set()
    for cn in _TOP_STRUCTS:
        s = sdfg.arrays.get(cn)
        if not isinstance(s, dt.Structure):
            continue
        for raw, mem in s.members.items():
            if isinstance(mem, dt.Scalar):
                out.add(f"{cn}.{_strip(raw)}")
    return out


def _map_param(
    name: str,
    bare: Dict[str, str],
    tss: Dict[int, str],
    arg_is_pointer: Dict[str, bool],
    scalar_struct_members: set,
) -> Optional[str]:
    """Map a stage1-frozen kernel param name to a C++ source expression.

    If the kernel takes the arg by pointer (``Array`` descriptor) but
    the corresponding C++ struct field is a value (``Scalar``), emit
    ``&<expr>`` so a length-1 array is produced from the field.
    """
    def _ptrify(expr: Optional[str]) -> Optional[str]:
        if expr is None:
            return None
        if not arg_is_pointer.get(name, False):
            return expr
        if expr in scalar_struct_members:
            return f"&{expr}"
        return expr
    # TSS_<N>: look up the original init RHS.
    m = _TSS_RE.match(name)
    if m:
        idx = int(m.group(1))
        return tss.get(idx)

    # __CG_<a>...__m_<x>: container path, top-level uses ``.`` then ``->``.
    cg = _split_cg(name)
    if cg is not None:
        path, member = cg
        if not path:
            return f"/* unknown CG: {name} */"
        if len(path) == 1:
            return _ptrify(f"{path[0]}.{member}")
        return _ptrify(f"{path[0]}." + "->".join(path[1:]) + f"->{member}")

    # Nested struct extent: ``<KIND>_<rest>_<container>_<num>`` where
    # ``<rest>`` ends with the inner-struct name (cells / edges / ...).
    m = _NESTED_RE.match(name)
    if m:
        prefix, body, container, _num = m.group(1), m.group(2), m.group(3), m.group(4)
        if "_" in body:
            head, inner = body.rsplit("_", 1)
            stripped = f"{prefix}_{head}"
            return f"{container}.{inner}->{stripped}"
        return f"/* nested-fallthrough: {name} */ 0"

    # Top-level array meta query.
    m = _TOP_AOA_RE.match(name)
    if m:
        kind, arr, dim = m.group(1), m.group(2), m.group(3)
        method = "size" if kind == "A" else "lbound"
        if arr in _TOP_ARRAYS:
            return f"serde::ARRAY_META_DICT_AT({arr}).{method}.at({dim})"
        return None

    # Bare struct member (``nlev`` / ``id`` / ...).
    if name in bare:
        return _ptrify(f"{bare[name]}.{name}")

    # Bare top-level array passed by name (no struct wrapper).
    if name in _TOP_ARRAYS:
        return name

    return None


# ---------- Signature extraction ----------------------------------------

def _init_param_names(sdfg: dace.SDFG) -> List[str]:
    """Return ``__dace_init_*`` arg names in the order DaCe emits them."""
    init = sdfg.init_signature(free_symbols=sdfg.free_symbols)
    if not init:
        return []
    out: List[str] = []
    for tok in init.split(","):
        tok = tok.strip()
        if not tok:
            continue
        out.append(tok.rsplit(None, 1)[-1].lstrip("*"))
    return out


def _call_arg_names(sdfg: dace.SDFG) -> List[str]:
    """Return ``__program_*`` data-arg names in the order DaCe emits them."""
    return list(sdfg.arglist().keys())


# ---------- Header emission ---------------------------------------------

_HEADER_TEMPLATE = """\
// Auto-generated by tools/gen_call_site.py. Do not edit by hand.
//
// Two macros for the 4 velocity tendencies variants. Stage 1 freezes the
// kernel ABI -- ``unify_variant_signatures`` makes all 4 variants expose
// the same ``arglist()``, so one macro per entry fits all.

#ifndef __VELOCITY_TENDENCIES_CALL_VELOCITY_H__
#define __VELOCITY_TENDENCIES_CALL_VELOCITY_H__

#include "serde_velocity_no_nproma.h"

// Per-variant entry-point declarations. Each header contributes
// ``__dace_init_<variant>``, ``__program_<variant>`` and
// ``__dace_exit_<variant>``. The build adds
// ``-I<sdfg.build_folder>/include`` for every variant, so these resolve
// without further -I flags from main.cpp.
{variant_includes}

#define VELOCITY_CALL_ARGS({macro_params}) \\
{call_body}

#define VELOCITY_INIT_ARGS({macro_params}) \\
{init_body}

#endif  // __VELOCITY_TENDENCIES_CALL_VELOCITY_H__
"""

_MACRO_PARAMS = (
    "global_data, p_diag, p_int, p_metrics, p_patch, p_prog, "
    "z_kin_hor_e, z_vt_ie, z_w_concorr_me, dt_linintp_ubc, "
    "dtime, istep, ldeepatmo, lvn_only, ntnd"
)


def _format_body(items: List[str]) -> str:
    if not items:
        return "    /* empty */"
    return ",\\\n".join(f"    {x}" for x in items)


def emit_header(
    sdfg_path: Path,
    unstripped_baseline: Path,
    variant_names: Optional[List[str]] = None,
) -> Tuple[str, List[str]]:
    """Build the call_velocity.h source. Returns ``(text, unmapped_names)``.

    ``variant_names`` is the list of stage1 SDFG names whose generated
    ``<name>.h`` headers should be ``#include``d (so ``main.cpp`` sees
    the four ``__dace_init_*`` / ``__program_*`` / ``__dace_exit_*``
    forward declarations through the single ``call_velocity.h`` include).
    Pass ``None`` to skip the include block (back-compat).
    """
    sdfg = dace.SDFG.from_file(str(sdfg_path))
    bare = _bare_members(unstripped_baseline)
    tss = _tss_map(unstripped_baseline)
    scalar_struct = _scalar_struct_members(unstripped_baseline)

    init_params = _init_param_names(sdfg)
    call_params = _call_arg_names(sdfg)

    # Build ``{arg_name: is_pointer_in_kernel_sig}`` for the call params.
    # Init params are always integer-by-value, so they don't need ptrify.
    arg_is_pointer: Dict[str, bool] = {}
    arglist = sdfg.arglist()
    for n, desc in arglist.items():
        arg_is_pointer[n] = isinstance(desc, dt.Array)

    unmapped: List[str] = []
    init_rows: List[str] = []
    for n in init_params:
        expr = _map_param(n, bare, tss, {}, scalar_struct)
        if expr is None:
            unmapped.append(n)
            init_rows.append(f"/* unknown: {n} */ 0")
        else:
            init_rows.append(expr)

    call_rows: List[str] = []
    for n in call_params:
        expr = _map_param(n, bare, tss, arg_is_pointer, scalar_struct)
        if expr is None:
            # ``arglist()`` includes scalars too; many of those are caller-
            # passed (dtime / istep / lvn_only / ntnd / ldeepatmo) so
            # match the macro-param names verbatim.
            call_rows.append(n)
        else:
            call_rows.append(expr)

    if variant_names:
        includes = "\n".join(f'#include "{n}.h"' for n in variant_names)
    else:
        includes = "// (no per-variant includes requested)"

    text = _HEADER_TEMPLATE.format(
        macro_params=_MACRO_PARAMS,
        init_body=_format_body(init_rows),
        call_body=_format_body(call_rows),
        variant_includes=includes,
    )
    return text, unmapped


def main(argv: Optional[List[str]] = None):
    argp = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    argp.add_argument(
        "--baseline",
        default="codegen/stage1/velocity_no_nproma_if_prop_lvn_only_0_istep_1.sdfgz",
        help="Frozen stage1 SDFG to read the kernel arglist from.",
    )
    argp.add_argument(
        "--unstripped",
        default="baseline/velocity_no_nproma.sdfgz",
        help="Pre-flatten baseline (for bare-member discovery + TSS init lookup).",
    )
    argp.add_argument("--output", default="include/call_velocity.h")
    args = argp.parse_args(argv)

    sdfg_path = Path(args.baseline)
    if not sdfg_path.is_file():
        sys.stderr.write(f"[gen_call_site] missing input: {sdfg_path}\n")
        sys.exit(1)
    unstripped = Path(args.unstripped)
    if not unstripped.is_file():
        sys.stderr.write(f"[gen_call_site] missing unstripped baseline: {unstripped}\n")
        sys.exit(1)

    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    # Discover sibling variant SDFGs in the same codegen directory so the
    # generated header pulls in their per-variant ``__dace_init_*`` /
    # ``__program_*`` / ``__dace_exit_*`` forward decls.
    variant_names = sorted({p.stem for p in sdfg_path.parent.glob("*.sdfgz")})

    text, unmapped = emit_header(sdfg_path, unstripped, variant_names=variant_names)
    out_path.write_text(text)
    print(f"[gen_call_site] wrote {out_path}")
    if unmapped:
        print(f"  WARNING: {len(unmapped)} unmapped init param(s); first 5:",
              file=sys.stderr)
        for n in unmapped[:5]:
            print(f"    {n}", file=sys.stderr)


if __name__ == "__main__":
    main()
