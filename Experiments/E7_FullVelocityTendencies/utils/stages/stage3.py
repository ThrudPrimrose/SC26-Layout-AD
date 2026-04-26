"""Stage 3: pre-GPU structural prep.

Four small steps, in order:

1. ``set_transient_sdfg_lifetime`` -- every transient array gets
   ``AllocationLifetime.SDFG`` so its storage is bound to the SDFG
   invocation (not a narrower scope). Persistent lifetime is deferred
   until the replacement GPU codegen provides init/exit bookends.
2. ``int64_to_int32`` -- demote 64-bit integer symbols, array dtypes
   and nested-SDFG connector types to 32-bit. Fixes the
   ``long int -> int`` truncation warnings emitted by the stage-2 code.
3. ``lift_transients`` -- multi-element transient arrays that live in
   nested SDFGs are promoted to the outermost SDFG's array table and
   piped back in as input/output connectors (routed through any
   enclosing MapEntry / MapExit). Keeps storage at a scope that GPU
   allocation can hook into.
4. ``verify_no_nested_transients`` -- explicit post-check that every
   multi-element transient now lives at the root. Scalars are fine.

No schedule assignment here (that's stage 4), no Persistent lifetime,
no symbol-propagation / reassign-vars.

Run:
    python -m utils.stages.stage3              # optimise + compile
    python -m utils.stages.stage3 --optimize   # codegen stage3 .sdfgz only
    python -m utils.stages.stage3 --compile    # compile stage3 .sdfgz only
"""

import argparse
from pathlib import Path

from utils.dace_branch import YAKUP_DEV_BRANCH, ensure_branch
ensure_branch(YAKUP_DEV_BRANCH)

import dace

from dace.transformation.passes.lift_transients import lift_transients
from dace.transformation.passes.loop_invariant_code_motion import (
    LoopInvariantCodeMotion)
from dace.transformation.passes.loop_local_memory_reduction import (
    LoopLocalMemoryReduction)
from dace.transformation.passes.verify_no_nested_transients import (
    verify_no_nested_transients)
from utils.passes.int64_to_int32 import int64_to_int32
from utils.passes.set_transient_sdfg_lifetime import set_transient_sdfg_lifetime
from utils.stages import common


STAGE_ID = 3


# Block-loop bounds in velocity_tendencies are computed by interstate
# edges (``i_startblk = p_patch%<grid>%start_block(rl)``), so lifting a
# transient through one of those maps would leave the lifted shape
# dependent on a symbol unknown at SDFG entry. ``lift_transients``
# accepts these as ``(begin_sym, end_sym) -> replacement`` pairs, but
# the ``i_startblk_var_<N>`` indices drift between runs (they're a
# product of simplification ordering, not source line numbers). The
# transient *names* don't drift -- they come from velocity.f90 -- so
# we keep a per-array map here and discover each transient's actual
# loop-bound pair at runtime.
#
# Add an entry whenever lift_transients reports a new transient whose
# block dim depends on an interstate-assigned ``i_(start|end)blk``
# pair. The replacement is which grid the transient is sized over
# (``nblks_c`` for cells, ``nblks_e`` for edges, ``nblks_v`` for
# verts).
_TRANSIENT_BLOCK_DIM = {
    "z_w_concorr_mc":  "__CG_p_patch__m_nblks_c",
    "z_w_con_c":       "__CG_p_patch__m_nblks_c",
    "z_w_con_c_full":  "__CG_p_patch__m_nblks_c",
    "cfl_clipping":    "__CG_p_patch__m_nblks_c",
    "maxvcfl":         "__CG_p_patch__m_nblks_c",
    "levmask":         "__CG_p_patch__m_nblks_c",
    "z_ekinh":         "__CG_p_patch__m_nblks_c",
    "z_v_grad_w":      "__CG_p_patch__m_nblks_e",
    "z_w_v":           "__CG_p_patch__m_nblks_v",
    "zeta":            "__CG_p_patch__m_nblks_v",
}


def _resolve_loop_pair_suggestions(sdfg: dace.SDFG, by_array):
    """Translate the per-array ``{name: nblks_*}`` table into the
    ``{(begin_sym, end_sym): nblks_*}`` form ``lift_transients`` wants.

    ``lift_transients`` keys its range-suggestions dict by the
    ``(begin, end)`` symbol pair extracted from the **enclosing map's
    range** (see ``_range_key`` in upstream pass) at the point of
    lifting. The transient's own descriptor doesn't carry those
    symbols yet -- they're prepended by ``_extend_shape_with_map_dims``
    only once the lift happens.

    So we walk every map in every nested SDFG. For each MapEntry whose
    body (transitively) writes/reads any transient listed in
    ``by_array``, we read the map's range and, for each (rb, re) pair
    that simplifies to single-symbol begin/end, register that pair
    against the array's replacement extent.
    """
    from dace.sdfg import nodes
    out = {}

    def _state_writes(state, name):
        """True if ``state`` (or any nested SDFG inside it) writes
        ``name`` as a top-level access in its enclosing SDFG."""
        for n in state.nodes():
            if isinstance(n, nodes.AccessNode) and n.data == name:
                return True
            if isinstance(n, nodes.NestedSDFG):
                if name in n.sdfg.arrays:
                    return True
                for st in n.sdfg.states():
                    if _state_writes(st, name):
                        return True
        return False

    for owner in sdfg.all_sdfgs_recursive():
        # Names that owner's tree (this SDFG + nested) declares as
        # transients we care about.
        relevant = {n for n in by_array if n in owner.arrays
                    and getattr(owner.arrays[n], "transient", False)}
        # Also pick up names declared in nested SDFGs we'll recurse into
        for state in owner.states():
            for n in state.nodes():
                if isinstance(n, nodes.NestedSDFG):
                    relevant |= {x for x in by_array if x in n.sdfg.arrays
                                 and getattr(n.sdfg.arrays[x], "transient", False)}
        if not relevant:
            continue

        for state in owner.states():
            for node in state.nodes():
                if not isinstance(node, nodes.MapEntry):
                    continue
                # Restrict to outermost maps (ones whose enclosing scope
                # is the state itself) -- that's where lift_transients
                # appends a dim.
                if state.entry_node(node) is not None:
                    continue
                # Does this map's body touch any relevant transient?
                touched = {n for n in relevant
                           if _state_writes(state, n)
                           and any(_subgraph_uses(state, node, n))}
                if not touched:
                    # Fall back to "any relevant transient touched" (the
                    # block loop is the only outer map that lifts these).
                    touched = relevant
                for rb, re_, _ in node.map.range.ndrange():
                    rb_syms = [str(s) for s in (getattr(rb, "free_symbols", None) or [])]
                    re_syms = [str(s) for s in (getattr(re_, "free_symbols", None) or [])]
                    if len(rb_syms) != 1 or len(re_syms) != 1:
                        continue
                    starts = [s for s in rb_syms if s.startswith("i_startblk")]
                    ends = [s for s in re_syms if s.startswith("i_endblk")]
                    if len(starts) != 1 or len(ends) != 1:
                        continue
                    pair = (starts[0], ends[0])
                    # Pick a replacement -- if multiple touched arrays
                    # disagree, prefer cells > edges > verts (the cell-
                    # block loop is the dominant one here).
                    pref = ["__CG_p_patch__m_nblks_c",
                            "__CG_p_patch__m_nblks_e",
                            "__CG_p_patch__m_nblks_v"]
                    candidates = {by_array[n] for n in touched}
                    for choice in pref:
                        if choice in candidates:
                            out.setdefault(pair, choice)
                            break
                    else:
                        out.setdefault(pair, next(iter(candidates)))
    return out


def _subgraph_uses(state, map_entry, name):
    """Yield True at least once if any AccessNode named ``name`` (or a
    nested SDFG whose arrays include ``name``) sits inside the scope
    rooted at ``map_entry``."""
    from dace.sdfg import nodes
    sub = state.scope_subgraph(map_entry)
    for n in sub.nodes():
        if isinstance(n, nodes.AccessNode) and n.data == name:
            yield True
            return
        if isinstance(n, nodes.NestedSDFG) and name in n.sdfg.arrays:
            yield True
            return


def optimization_action(sdfg: dace.SDFG) -> dace.SDFG:
    # 1. Transient lifetime.
    flipped = set_transient_sdfg_lifetime(sdfg)
    if flipped:
        print(f"Stage #{STAGE_ID}: set_transient_sdfg_lifetime updated {flipped} descriptor(s)")
    sdfg.validate()

    # 2. Integer width demotion.
    demoted = int64_to_int32(sdfg)
    if demoted:
        print(f"Stage #{STAGE_ID}: int64_to_int32 demoted {demoted} type(s)")
    sdfg.validate()

    # 3. Lift cross-scope / multi-element transients. The pass checks
    # its own post-conditions at the end and raises on violation. The
    # ``(begin, end) -> nblks_*`` table is built fresh from the per-
    # array map: simplify reorderings shift the ``i_(start|end)blk``
    # indices between runs, but the transient names are stable.
    range_suggestions = _resolve_loop_pair_suggestions(sdfg, _TRANSIENT_BLOCK_DIM)
    if range_suggestions:
        print(f"Stage #{STAGE_ID}: resolved {len(range_suggestions)} loop-pair "
              f"suggestion(s) for lift_transients")
    lifted = lift_transients(sdfg, map_range_suggestions=range_suggestions)
    if lifted:
        print(f"Stage #{STAGE_ID}: lift_transients promoted {lifted} array(s) to top level")
    sdfg.validate()

    # 4. Independent invariant check: every multi-element transient
    # must now live at the root SDFG. Scalars (shape == (1,)) are left
    # alone. Redundant with lift_transients's internal check, but
    # explicit here so the pipeline stays guarded even if a future
    # step re-introduces a nested transient (e.g. a GPU pass adding a
    # scratch array at the wrong level).
    verify_no_nested_transients(sdfg)

    # 5. Loop-invariant code motion. Hoists tasklets whose inputs don't
    # change across iterations out of enclosing LoopRegions (into the
    # preheader) and out of Map scopes (into the enclosing state).
    # Runs before GPU offloading so the hoisted tasklets are scheduled
    # on the host and the per-thread kernel work shrinks.
    hoisted = LoopInvariantCodeMotion().apply_pass(sdfg, {})
    if hoisted:
        print(f"Stage #{STAGE_ID}: LICM hoisted {hoisted} tasklet/region(s)")
    sdfg.validate()

    # 6. Loop-local memory reduction. Shrinks thread-local arrays whose
    # access pattern is a sliding window (``a[i + k]``) to a circular
    # buffer of size ``max(k) - min(k) + 1``. Rounds up to the next
    # power of two so modulo becomes a bitmask. Per-iteration memory
    # drops from O(N) to O(window size); a win for any stencil-style
    # transient that survived earlier simplification.
    shrunk = LoopLocalMemoryReduction().apply_pass(sdfg, {})
    if shrunk:
        print(f"Stage #{STAGE_ID}: LLMR reduced {len(shrunk)} array(s)")
    sdfg.validate()

    return sdfg


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
        for name in names:
            infile = common.stage_input(name, STAGE_ID)
            outfile = common.stage_output(name, STAGE_ID)
            print(f"Stage #{STAGE_ID}: Optimising {name} from {infile}")

            sdfg = dace.SDFG.from_file(infile)
            sdfg.name = name
            sdfg.validate()

            sdfg = optimization_action(sdfg)

            Path(outfile).parent.mkdir(parents=True, exist_ok=True)
            sdfg.save(outfile, compress=True)
            print(f"Stage #{STAGE_ID}: Saved as {outfile}")

    if args.compile:
        sdfgs = {
            name: dace.SDFG.from_file(common.stage_output(name, STAGE_ID))
            for name in names
        }
        common.compile_action(STAGE_ID, sdfgs, release=args.release)


if __name__ == "__main__":
    main()
