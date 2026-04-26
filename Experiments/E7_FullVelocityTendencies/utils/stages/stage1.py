"""Stage 1: lift scalar-accumulator loops to ``dace.libraries.standard.Reduce``.

Uses ``dace.transformation.passes.loop_to_reduce.LoopToReduce`` (available on
``yakup/dev``). The pass detects three shapes:

- Tasklet body ``acc = acc <op> arr[f(i)]``       -> add/mul/bitwise/max/min
- Interstate edge ``{sym: sym <op> arr[f(i)]}``    -> add/mul
- ``ConditionalBlock`` guarded by ``sym <cmp> arr[f(i)]`` + empty states +
  ``{sym: arr[f(i)]}``                             -> max/min

For now we let DaCe emit its default ``Reduce`` codegen (stock tree / warp
reductions depending on storage). A later stage will swap the emitted
``Reduce`` for a custom library node that dispatches to our ``reduce_<op>_{cpu,
gpu,device}`` implementations and requires ``init_reduce_*()`` /
``cleanup_reduce_*()`` bookends -- that change needs paired init/exit-state
passes not yet written.

Run:
    python -m utils.stages.stage1              # optimise + compile
    python -m utils.stages.stage1 --optimize   # codegen stage1 .sdfgz only
    python -m utils.stages.stage1 --compile    # compile stage1 .sdfgz only
"""

import argparse
from pathlib import Path

from utils.dace_branch import YAKUP_DEV_BRANCH, ensure_branch
ensure_branch(YAKUP_DEV_BRANCH)

import dace
from dace import nodes
from dace.sdfg.propagation import propagate_memlets_sdfg
from dace.sdfg.sdfg_construction_utils import prune_unused_nsdfg_connectors_recursive
from dace.transformation.dataflow import MapCollapse
from dace.transformation.dataflow.perf_loop_nesting import PerfLoopNesting
from dace.transformation.interstate import LoopToMap
from dace.transformation.interstate.move_if_into_map import MoveIfIntoMap
from dace.transformation.interstate.sdfg_nesting import InlineSDFG
from dace.transformation.passes.loop_to_reduce import LoopToReduce
from dace.transformation.passes.simplification.continue_to_condition import ContinueToCondition

from utils.passes.baseline_fix import fix_baseline
from utils.passes.promote_maxvcfl import promote_maxvcfl
from utils.passes.remove_clip_count import remove_clip_count
from utils.passes.uniquify_difcoef import uniquify_difcoef
from utils.stages import common


STAGE_ID = 1


def optimization_action(sdfg: dace.SDFG) -> dace.SDFG:
    # 0. Strip the ``istep`` / ``lvn_only`` Scalar duplicates f2dace left in
    #    ``sdfg.arrays`` -- yakup/dev's validator rejects them; FaCe
    #    tolerated them.
    fixed = fix_baseline(sdfg)
    if fixed:
        print(f"Stage #{STAGE_ID}: baseline-fix removed {fixed} stale scalar(s)")



    # 0b. Specialise away the ``clip_count`` accumulator. The full pipeline
    #     assumes the clipping check always fires, so the counter, its
    #     ``if clip_count == 0: continue`` guard, and the predicate scalar
    #     feeding it are all dead. Run before LoopToReduce so the lifted
    #     reductions don't have to thread ``clip_count`` through their
    #     symbol maps.
    dropped = remove_clip_count(sdfg)
    assert dropped
    if dropped:
        print(f"Stage #{STAGE_ID}: remove_clip_count dropped {dropped} assignment(s)")
    sdfg.validate()

    # 1. Lift scalar-accumulator loops to Reduce before anything else touches
    #    them -- LoopToMap would parallelise them and make the pattern-match
    #    impossible afterwards.
    lifted = LoopToReduce(permissive=True).apply_pass(sdfg, {})
    print(f"Stage #{STAGE_ID}: lifted {lifted or 0} loop(s) to Reduce in {sdfg.name}")
    sdfg.validate()
    # 1b. Promote the ``maxvcfl`` scalar accumulator into a per-nproma array
    #     and install a final ``Reduce(max)`` into ``vcflmax``. Both are
    #     transients, so the SDFG signature is unchanged. Done before
    #     ``ContinueToCondition`` so the newly-inserted reduction is visible
    #     to the downstream simplify / LoopToMap passes.
    promote_maxvcfl(sdfg)
    sdfg.validate()
    # 1c. Fold ``continue`` blocks into guard conditions so LoopToMap sees
    #     plain if/else instead of a control-flow exit.
    ContinueToCondition().apply_pass(sdfg, {})
    sdfg.validate()

    # 1d. Uniquify every ``difcoef`` occurrence into its own thread-local
    #     transient (``difcoef0``, ``difcoef1``, ...). Removes the false
    #     sharing that would otherwise block ``LoopToMap`` with read-write
    #     conflict errors on the shared scratch scalar.
    renamed = uniquify_difcoef(sdfg)
    assert renamed
    print(f"Stage #{STAGE_ID}: uniquify_difcoef split into {renamed} thread-local copies")
    sdfg.validate()

    # 2. Simplify: after AoS->SoA + reductions there's a lot of foldable
    #    structure -- dead states, unused assignments, collapsible control
    #    flow. ``ArrayElimination`` is the standing f2dace-workaround skip.
    sdfg.simplify(skip=["ArrayElimination"])
    sdfg.validate()

    # 2b. Propagate memlets so every edge carries its tightest possible
    #     subset. LoopToMap's read-write conflict checks rely on precise
    #     memlets -- without propagation, aggregate map-exit memlets
    #     default to the full array range and block parallelisation.
    propagate_memlets_sdfg(sdfg)
    sdfg.validate()

    # 3. Max parallelism: turn as many loops as possible into maps.
    #    ``permissive=True`` matches icon-artifacts stage 2 behaviour.
    sdfg.apply_transformations_repeated(LoopToMap, permissive=True)
    sdfg.simplify(skip=["ArrayElimination"])

    # 3b. Duplicate the parent map around each inner child for
    #     `_for_it_36`. This is the imperfect-nest motif that the
    #     CFL-clipping kernel uses; splitting it here lets collapse /
    #     codegen handle each sibling independently. Apply only to
    #     outer maps parameterised by `_for_it_36` so we don't
    #     fission unrelated maps.
    pln_count = 0
    while True:
        applied_one = False
        # _for_it_36 lives in a nested SDFG (the loop body f2dace
        # emits), so we must scan every SDFG and call
        # can_be_applied_to / apply_to with the *owning* SDFG -- not
        # the top-level one.
        for owner in list(sdfg.all_sdfgs_recursive()):
            for state in owner.all_states():
                for n in list(state.nodes()):
                    if isinstance(n, nodes.MapEntry) and "_for_it_36" in n.map.params:
                        if PerfLoopNesting().can_be_applied_to(owner, parent_entry=n):
                            PerfLoopNesting().apply_to(owner, parent_entry=n)
                            pln_count += 1
                            applied_one = True
                            break
                if applied_one:
                    break
            if applied_one:
                break
        if not applied_one:
            break
    if pln_count > 0:
        print(f"Stage #{STAGE_ID}: PerfLoopNesting fissioned {pln_count} `_for_it_36` map(s)")
    else:
        # Optimization, not a correctness step: when upstream fixes
        # (e.g. struct_to_container_group / dealias_symbols cleanup)
        # shift the CFL-clipping kernel's nest shape,
        # ``can_be_applied_to`` may return False for every
        # ``_for_it_36`` MapEntry. Continue without fissioning -- the
        # rest of stage 1 still produces a valid SDFG.
        print(f"Stage #{STAGE_ID}: PerfLoopNesting matched 0 `_for_it_36` maps "
              "(no imperfect-nest motif to fission; continuing)")
    sdfg.validate()

    # 3c. Push conditionals guarding an inner map into the inner nested SDFG
    #     so outer and inner maps become direct neighbours. Apply repeatedly
    #     everywhere it matches (the transformation's ``can_be_applied`` is
    #     the eligibility filter).
    moved_if = sdfg.apply_transformations_repeated(MoveIfIntoMap)
    if moved_if:
        print(f"Stage #{STAGE_ID}: MoveIfIntoMap pushed {moved_if} conditional(s) inside map bodies")
    sdfg.validate()

    # 3d. Clean unused in/out connectors on nested SDFGs sitting inside maps.
    #     Dead connectors block InlineSDFG and balloon the symbol maps; the
    #     velocity pipeline previously relied on a hand-written cleaner for
    #     this, now upstreamed to `sdfg_construction_utils`.
    pruned = prune_unused_nsdfg_connectors_recursive(sdfg)
    if pruned:
        print(f"Stage #{STAGE_ID}: pruned {pruned} unused nested-SDFG connector(s)")
    sdfg.validate()

    # 3e. Inline as much as possible before collapse so neighbouring maps end
    #     up in the same state and are visible to MapCollapse.
    inlined = sdfg.apply_transformations_repeated(InlineSDFG)
    if inlined:
        print(f"Stage #{STAGE_ID}: InlineSDFG inlined {inlined} nested SDFG(s)")
    sdfg.simplify(skip=["ArrayElimination"])
    sdfg.validate()

    # 4. Collapse nested maps, then one more simplify to tidy up.
    sdfg.apply_transformations_repeated(MapCollapse)
    sdfg.simplify(skip=["ArrayElimination"])
    sdfg.validate()

    # 5. Fold ``__CG_p_patch__m_nlev`` to the literal 90 (and nlevp1
    #    to 91). For the data we run against (r02b05 / num_lev=90)
    #    these are compile-time constants. Folding has two effects:
    #      - kernel signatures shrink (nlev/nlevp1 leave the ABI),
    #      - inner ``__device__`` helpers no longer need to thread
    #        nlev through their parameter lists, which avoids a
    #        codegen scope-leak in stage 4 (nlev referenced inside a
    #        device function whose parameter list omitted it).
    _fold_nlev(sdfg)
    sdfg.validate()
    return sdfg


def _fold_nlev(sdfg: dace.SDFG):
    """Specialise ``__CG_p_patch__m_nlev`` -> 90 and
    ``__CG_p_patch__m_nlevp1`` -> 91 across the SDFG tree, then drop
    the now-dead symbol/array entries. Idempotent (a second call sees
    no occurrences and does nothing)."""
    folds = {
        "__CG_p_patch__m_nlev": "90",
        "__CG_p_patch__m_nlevp1": "91",
    }
    n_replaced = 0
    for nested in sdfg.all_sdfgs_recursive():
        present = {k: v for k, v in folds.items()
                   if k in nested.symbols or k in nested.arrays
                   or k in {str(s) for s in nested.free_symbols}}
        if not present:
            continue
        nested.replace_dict(present)
        n_replaced += len(present)
        for k in present:
            if k in nested.arrays:
                nested.remove_data(k, validate=False)
            if k in nested.symbols:
                del nested.symbols[k]
    if n_replaced:
        print(f"Stage #{STAGE_ID}: folded nlev/nlevp1 to literal "
              f"({n_replaced} occurrence(s) across the SDFG tree)")


def main():
    argp = argparse.ArgumentParser()
    argp.add_argument("--optimize", action=argparse.BooleanOptionalAction, default=False)
    argp.add_argument("--compile", action=argparse.BooleanOptionalAction, default=False)
    mode = argp.add_mutually_exclusive_group()
    mode.add_argument("--release", dest="release", action="store_true",
                      help="build with -O3 + --use_fast_math (FMA may diverge from IEEE)")
    mode.add_argument("--debug", dest="release", action="store_false",
                      help="build with -O0 + DACE_VELOCITY_DEBUG + IEEE FP (default)")
    argp.set_defaults(release=False)
    args = argp.parse_args()
    if not args.optimize and not args.compile:
        args.optimize, args.compile = True, True

    names = common.sdfg_names()

    if args.optimize:
        # Stage 1 is the freeze point for the kernel ABI. Stage 1's
        # optimisations (LoopToReduce / LoopToMap / InlineSDFG /
        # simplify) surface additional free symbols (typically the d_2
        # shape extents that LoopToMap exposes) and prune unused
        # arrays, so the per-variant signature drifts from the
        # post-phase-1 baseline. We do **not** try to re-pad back to
        # the baseline -- whatever stage 1 produces *is* the frozen
        # ABI, and stages 2-6 must preserve it.
        from utils.passes.unify_variant_signatures import unify_variant_signatures
        optimized = []
        for name in names:
            infile = common.stage_input(name, STAGE_ID)
            print(f"Stage #{STAGE_ID}: Optimising {name} from {infile}")
            sdfg = dace.SDFG.from_file(infile)
            sdfg.name = name
            sdfg.validate()
            sdfg = optimization_action(sdfg)
            optimized.append(sdfg)

        if optimized:
            # Unify the 4 variants against each other (no external
            # baseline). ``unify_variant_signatures`` pools the union
            # of every variant's arglist, pads each variant up to the
            # union, and asserts the 4 final arglists agree. Pass
            # ``optimized[0]`` as the unify reference so the helper
            # has a baseline parameter to satisfy its API; the union
            # is over all 4 anyway.
            print(f"Stage #{STAGE_ID}: freezing signature across "
                  f"{len(optimized)} variant(s)")
            unify_variant_signatures(optimized[0], optimized)
            for v in optimized:
                v.validate()

            sigs = [list(v.arglist().keys()) for v in optimized]
            ref = sigs[0]
            for v, s in zip(optimized[1:], sigs[1:]):
                if s != ref:
                    extra = set(s) - set(ref)
                    missing = set(ref) - set(s)
                    raise RuntimeError(
                        f"Stage #{STAGE_ID}: signature drift in {v.name!r} "
                        f"after unify. extra={sorted(extra)} "
                        f"missing={sorted(missing)}"
                    )

            # Persist the frozen signature so stages 2-6 can assert
            # they preserved it. Stage 1's output is the canonical ABI.
            sig_path = Path("codegen/stage1/__signature.txt")
            sig_path.parent.mkdir(parents=True, exist_ok=True)
            sig_path.write_text("\n".join(ref) + "\n")
            print(f"Stage #{STAGE_ID}: signature frozen ({len(ref)} args), "
                  f"saved to {sig_path}")

        for sdfg in optimized:
            outfile = common.stage_output(sdfg.name, STAGE_ID)
            Path(outfile).parent.mkdir(parents=True, exist_ok=True)
            sdfg.save(outfile, compress=True)
            print(f"Stage #{STAGE_ID}: Saved as {outfile}")

    if args.compile:
        sdfgs = {
            name: dace.SDFG.from_file(common.stage_output(name, STAGE_ID))
            for name in names
        }

        # Regenerate the C++ struct definitions and the call-site macros
        # from the frozen stage1 SDFG before compile. ``_s_<NNN>`` and
        # ``__f2dace_*`` are stripped by RenameStrippedSymbols on the
        # SDFG side, so both header generators emit names in stripped
        # form -- no drift between the kernel signature, the struct
        # member set, and the macro arglist.
        import re as _re
        from tools.gen_struct_headers import emit_header as _emit_struct_header
        from tools.gen_call_site import emit_header as _emit_call_site

        repo = Path(__file__).resolve().parent.parent.parent
        baseline_unstripped = repo / "baseline" / "velocity_no_nproma.sdfgz"
        any_stage1 = repo / common.stage_output(next(iter(sdfgs)), STAGE_ID)

        struct_h = repo / "include" / "velocity_tendencies_no_nproma.h"
        struct_text, rename_log = _emit_struct_header(baseline_unstripped, strip=True)
        struct_h.write_text(struct_text)
        print(f"Stage #{STAGE_ID}: regenerated {struct_h.relative_to(repo)} "
              f"(stripped {len(rename_log)} member name(s))")

        call_h = repo / "include" / "call_velocity.h"
        call_text, unmapped = _emit_call_site(
            any_stage1, baseline_unstripped, variant_names=list(sdfgs.keys()),
        )
        call_h.write_text(call_text)
        print(f"Stage #{STAGE_ID}: regenerated {call_h.relative_to(repo)}")
        if unmapped:
            print(f"Stage #{STAGE_ID}: WARNING -- {len(unmapped)} unmapped "
                  f"init param(s) in call_velocity.h: {unmapped[:5]}")

        # Normalise the (de)serialization header in place. The committed
        # serde file refers to struct members by their original f2dace
        # names (``x->__f2dace_<KIND>_<arr>_d_<dim>_s_<NNN>``); apply the
        # same strip rules used by RenameStrippedSymbols so it lines up
        # with the regenerated struct definitions. Idempotent.
        serde_h = repo / "include" / "serde_velocity_no_nproma.h"
        if serde_h.exists():
            txt = serde_h.read_text()
            new = _re.sub(r"__f2dace_([A-Za-z][A-Za-z0-9]*)_", r"\1_", txt)
            new = _re.sub(r"_s_\d+", "", new)
            if new != txt:
                serde_h.write_text(new)
                stripped = sum(1 for _ in _re.finditer(
                    r"__f2dace_[A-Za-z][A-Za-z0-9]*_|_s_\d+", txt))
                print(f"Stage #{STAGE_ID}: stripped serde header in place "
                      f"({stripped} occurrence(s))")

        common.compile_action(STAGE_ID, sdfgs, release=args.release)


if __name__ == "__main__":
    main()
