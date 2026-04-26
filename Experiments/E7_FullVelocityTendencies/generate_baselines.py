"""Generate the 4 baseline SDFG variants from ``velocity_no_nproma.sdfgz``.

Pipeline (8 steps):

1. ``StructToContainerGroups`` (AoS -> SoA). Drops ``.cpp`` / ``.h``
   artifacts into CWD; these get moved into ``--output-dir`` at the end.
2. ``PrepareBaseline``: parses ``sdfg.global_code`` + ``sdfg.init_code``
   and registers every LHS as a global symbol (dtype from the C
   declaration, ``int32`` otherwise). No interstate-edge scaffold is
   installed; the raw ``{lhs: rhs}`` init map is returned for step 3.
   Flatten tasklets are also spliced, SoA leaves flipped non-transient.
3. ``DealiasSymbols``: for every ``tmp_struct_symbol_N`` whose init
   RHS matches the one-hop ``<container>.<member>`` shape, renames to
   ``__CG_<container>__m_<member>`` via ``sdfg.replace_dict``. The
   remaining ``__f2dace_*`` LHS stay as free kernel parameters.
4. ``ResolveExtentOffsets``: folds every ``SOA`` / ``OA`` offset symbol
   whose observed ``lbound`` is constantly 1 (per
   ``baseline/lbounds.csv`` produced by ``tools/analyze_lbounds``).
   The analyzer is auto-built / auto-run on ``--data-dir`` if the CSV
   is missing.
5. ``ResolveExtentSizes``: folds every ``SA`` / ``A`` size symbol to a
   well-known scalar (``nproma`` / ``nblks_c`` / ``nblks_e`` / ...).
   ``RenameStrippedSymbols`` follows: DaCe's framecode hardcodes a
   filter that drops ``__f2dace_SA*`` / ``__f2dace_SOA*`` /
   ``tmp_struct_symbol*`` names from the kernel signature, so we
   rename them to ``SA_<...>`` / ``SOA_<...>`` / ``TSS_<N>`` (and
   strip the per-occurrence ``_s_<idx>`` segment) before compile.
6. Save the pre-specialise baseline SDFG and compile it as a checkpoint.
7. ``specialize_vt`` x4 over ``{lvn_only, istep}``.
8. ``unify_variant_signatures``: pad every variant so the 4 arglists
   agree; ``unique_names`` disambiguates state labels; save each.

Must run on a DaCe branch that ships ``StructToContainerGroups``
(f2dace line). ``LiftTrivialIf`` is optional -- ``specialize_vt`` skips
it when absent.
"""

import argparse
import copy
import itertools
import re
import shutil
import subprocess
from pathlib import Path

import dace
from dace import data
from dace.transformation.passes import RemoveUnusedSymbols, StructToContainerGroups

from utils.passes.dealias_symbols import DealiasSymbols
from utils.passes.prepare_baseline import PrepareBaseline
from utils.passes.rename_stripped_symbols import RenameStrippedSymbols
from utils.passes.resolve_extent_offsets import ResolveExtentOffsets
from utils.passes.resolve_extent_sizes import ResolveExtentSizes
from utils.passes.unify_variant_signatures import unify_variant_signatures
from utils.unique_names import unique_names
from utils.clean_bad_views import clean_bad_views
from utils.specialize_vt import specialize_vt


_REPO = Path(__file__).resolve().parent
_ANALYZER_SRC = _REPO / "tools" / "analyze_lbounds.cpp"
_ANALYZER_BIN = _REPO / "tools" / "analyze_lbounds"


def _move_scg_artifacts(cwd: Path, dest: Path, sdfg_name: str):
    """Move ``<sdfg_name>_*flattener_code*.cpp|.h`` and similar files
    that StructToContainerGroups drops in ``cwd`` into ``dest``."""
    dest.mkdir(parents=True, exist_ok=True)
    for p in sorted(cwd.iterdir()):
        if not p.is_file() or p.suffix not in (".cpp", ".h"):
            continue
        if not p.name.startswith(sdfg_name + "_"):
            continue
        target = dest / p.name
        shutil.move(str(p), str(target))
        print(f"  moved {p.name} -> {target}")


def _run_aos_to_soa(sdfg: dace.SDFG):
    clean_bad_views(sdfg)

    StructToContainerGroups(
        validate=False,
        save_steps=False,
        verbose=False,
        simplify=False,
        interface_with_struct_copy=True,
        interface_to_gpu=False,
        clean_trivial_views=True,
        shallow_copy=False,
        shallow_copy_to_gpu=False,
        taskloop=False,
        dont_prune_unused_containers=True,
    ).apply_pass(sdfg, {})


def _ensure_analyzer_binary():
    src_mtime = _ANALYZER_SRC.stat().st_mtime
    if _ANALYZER_BIN.exists() and _ANALYZER_BIN.stat().st_mtime >= src_mtime:
        return
    print(f"Building {_ANALYZER_BIN.relative_to(_REPO)}")
    subprocess.run(
        ["g++", "-O3", "-std=c++17", str(_ANALYZER_SRC), "-o", str(_ANALYZER_BIN)],
        check=True,
    )


_EXCLUDED_SCALAR_ARGS = (
    # Output from the SCG-flattened ``p_diag`` struct. Must stay
    # non-transient (caller needs the written value) but shouldn't be a
    # by-value scalar parameter. Promoted to a length-1 non-transient
    # Array so it emits as a ``double *`` pointer parameter instead.
    "__CG_p_diag__m_max_vcfl_dyn",
)


def _promote_scalar_outputs_to_arrays(sdfg: dace.SDFG, names) -> int:
    """Inverse of ``dace.sdfg.construction_utils.replace_length_one_arrays_with_scalars``
    restricted to the given name set.

    For each name in ``names`` that is a non-transient ``Scalar`` on
    ``sdfg`` (or a nested SDFG): swap its descriptor for a length-1
    non-transient ``Array`` of the same dtype / storage / lifetime, and
    rewrite every bare-name reference to ``name[0]`` in:

    - interstate-edge assignments (RHS) and conditions
    - ``ConditionalBlock`` branch conditions
    - ``LoopRegion`` ``loop_condition`` / ``init_statement`` / ``update_statement``

    Memlets and access nodes don't need rewriting -- subset ``"0"``
    works for both ``Scalar`` and length-1 ``Array`` descriptors.

    Token-bounded match (``(?<!\\w)name(?!\\w)``) so partial identifier
    hits are left alone. Recurses into every nested SDFG. Returns the
    count of descriptors promoted.
    """
    from dace.properties import CodeBlock
    from dace.sdfg.state import ConditionalBlock, LoopRegion

    total = 0
    for nested in sdfg.all_sdfgs_recursive():
        promoted = set()
        for name in names:
            desc = nested.arrays.get(name)
            if isinstance(desc, data.Scalar) and not desc.transient:
                dtype = desc.dtype
                storage = desc.storage
                lifetime = desc.lifetime
                debuginfo = getattr(desc, "debuginfo", None)
                nested.remove_data(name, validate=False)
                nested.add_array(
                    name,
                    [1],
                    dtype,
                    transient=False,
                    storage=storage,
                    lifetime=lifetime,
                    debuginfo=debuginfo,
                    find_new_name=False,
                )
                promoted.add(name)
                total += 1

        if not promoted:
            continue

        patterns = {
            name: re.compile(r"(?<!\w)" + re.escape(name) + r"(?!\w)")
            for name in promoted
        }

        def _sub(text: str) -> str:
            out = text
            for name, pat in patterns.items():
                out = pat.sub(f"{name}[0]", out)
            return out

        # Interstate edges: assignment RHS + condition.
        for edge in nested.all_interstate_edges():
            assigns = edge.data.assignments or {}
            if assigns:
                edge.data.assignments = {
                    k: (_sub(v) if isinstance(v, str) else v)
                    for k, v in assigns.items()
                }
            if edge.data.condition is not None:
                old = edge.data.condition.as_string
                new = _sub(old)
                if new != old:
                    edge.data.condition = CodeBlock(new)

        # ConditionalBlock branch conditions.
        for block in nested.all_control_flow_blocks():
            if isinstance(block, ConditionalBlock):
                for i, (cond, body) in enumerate(block.branches):
                    if cond is None:
                        continue
                    new = _sub(cond.as_string)
                    if new != cond.as_string:
                        block.branches[i] = (CodeBlock(new), body)

        # LoopRegion expressions.
        for region in nested.all_control_flow_regions():
            if isinstance(region, LoopRegion):
                for attr in ("loop_condition", "init_statement", "update_statement"):
                    code = getattr(region, attr, None)
                    if code is None:
                        continue
                    new = _sub(code.as_string)
                    if new != code.as_string:
                        setattr(region, attr, CodeBlock(new))

    return total


def _assert_no_structs(sdfg: dace.SDFG):
    """Raise if any nested SDFG still carries a ``Structure`` data descriptor."""
    offenders = []
    for nested in sdfg.all_sdfgs_recursive():
        for name, desc in nested.arrays.items():
            if isinstance(desc, data.Structure):
                offenders.append(f"{nested.name}.{name} ({type(desc).__name__})")
    if offenders:
        raise AssertionError(
            f"{sdfg.name!r} still carries struct data descriptors:\n  "
            + "\n  ".join(offenders)
        )


def _ensure_lbound_csv(data_dir: Path, csv_path: Path):
    if csv_path.exists():
        return
    _ensure_analyzer_binary()
    print(f"Scanning {data_dir} -> {csv_path}")
    subprocess.run(
        [str(_ANALYZER_BIN), str(data_dir), str(csv_path)],
        check=True,
    )


def main():
    argp = argparse.ArgumentParser()
    argp.add_argument("--input", default="baseline/velocity_no_nproma.sdfgz")
    argp.add_argument("--output-dir", default="baseline")
    argp.add_argument(
        "--data-dir", default="data_r02b05", help="R02B05 folder for lbound analysis"
    )
    argp.add_argument("--lbounds-csv", default="baseline/lbounds.csv")
    args = argp.parse_args()

    out_dir = Path(args.output_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    cwd = Path.cwd().resolve()

    print(f"Loading {args.input}")
    sdfg = dace.SDFG.from_file(args.input)
    sdfg.name = Path(args.input).stem
    sdfg.validate()

    # Compile original one as reference
    # copysdfg = copy.deepcopy(sdfg)
    # copysdfg.name += "_aos"
    # copysdfg.compile()

    print("1/8  StructToContainerGroups (AoS -> SoA)")
    _run_aos_to_soa(sdfg)
    _move_scg_artifacts(cwd, out_dir, sdfg.name)

    print("2/8  PrepareBaseline")
    init_map = PrepareBaseline().apply_pass(sdfg, {}) or {}
    print(f"  registered {len(init_map)} init-code LHS as int32 symbols")

    print("3/8  DealiasSymbols")
    dealiased = DealiasSymbols(init_map=init_map).apply_pass(sdfg, {})
    if dealiased:
        print(f"  aliased {len(dealiased)} tmp_struct_symbol_* to mangled names")

    csv = Path(args.lbounds_csv).resolve()
    print(f"4/8  ResolveExtentOffsets ({csv.name})")
    _ensure_lbound_csv(Path(args.data_dir).resolve(), csv)
    folded = ResolveExtentOffsets(csv_path=str(csv)).apply_pass(sdfg, {})
    if folded:
        print(f"  folded {len(folded)} offset symbols to 1")

    print(f"5/8  ResolveExtentSizes ({csv.name})")
    sized = ResolveExtentSizes(csv_path=str(csv)).apply_pass(sdfg, {})
    if sized:
        print(f"  folded {len(sized)} size symbols to scalar names")

    # DaCe's framecode hardcodes a filter that drops every
    # ``__f2dace_SA*`` / ``__f2dace_SOA*`` / ``tmp_struct_symbol*`` name
    # from the kernel signature (dace/codegen/targets/framecode.py:55-60,
    # marked "TODO: Hack, remove!"). We can't touch that from outside, so
    # rename our symbols to forms (``SA_<...>``, ``SOA_<...>``, ``TSS_<N>``)
    # that slip past the filter; also strips the per-occurrence
    # ``_s_<idx>`` segment.
    renamed = RenameStrippedSymbols().apply_pass(sdfg, {}) or {}
    if renamed:
        print(f"  renamed {len(renamed)} stripped-prefix symbols (framecode hack)")

    n_promoted = _promote_scalar_outputs_to_arrays(sdfg, _EXCLUDED_SCALAR_ARGS)
    if n_promoted:
        print(f"  promoted {n_promoted} scalar output(s) to length-1 Array "
              f"(pointer param): {list(_EXCLUDED_SCALAR_ARGS)}")

    pre_spec = out_dir / f"{sdfg.name}_post_aos_soa.sdfgz"
    print(f"6/8  saving pre-specialise baseline -> {pre_spec}")
    # Test here by compiling
    sdfg.save(pre_spec, compress=True)
    sdfg.validate()
    sdfg.compile()

    print("7/8  specialize_vt x4 configs")
    variants = []
    for lvn_only, istep in itertools.product(("0", "1"), ("1", "2")):
        cfg = {"lvn_only": lvn_only, "istep": istep}
        print(f"     {cfg}")
        variants.append(specialize_vt(sdfg, cfg))

    for variant in variants:
        RemoveUnusedSymbols().apply_pass(variant, {})  # fold away dead symbols from specialization
        # Rerun the stripped-prefix rename: specialize_vt deepcopies pre-rename
        # content out of ``sdfg``, but ``simplify`` inside it may resurrect
        # SA/SOA names from latent array-shape expressions. Idempotent no-op
        # if nothing matches.
        RenameStrippedSymbols().apply_pass(variant, {})
        _promote_scalar_outputs_to_arrays(variant, _EXCLUDED_SCALAR_ARGS)
        # Test here by compiling
        variant.save(out_dir / f"{variant.name}.sdfgz", compress=True)
        variant.validate()
        variant.compile()

    print("8/8  unify_variant_signatures (arglist must match baseline)")
    unify_variant_signatures(sdfg, variants)
    for v in variants:
        v.validate()
        _assert_no_structs(v)

    unique_names(variants)
    for v in variants:
        # ``unify_variant_signatures`` may have padded fresh SA/SOA names
        # pulled from siblings -- rename one more time before the final
        # compile to catch them.
        RenameStrippedSymbols().apply_pass(v, {})
        _promote_scalar_outputs_to_arrays(v, _EXCLUDED_SCALAR_ARGS)
        path = out_dir / f"{v.name}.sdfgz"
        v.save(path, compress=True)
        v.validate()
        v.compile()
        print(f"  -> {path}")


if __name__ == "__main__":
    main()
