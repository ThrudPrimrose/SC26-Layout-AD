"""Stage 4: GPU offload.

Applies ``OffloadVelocityToGPU`` to each stage-3 SDFG:
- mirrors every non-transient CPU-storage Array to a ``gpu_<name>``
  sibling (transient) with copy-in/copy-out states -- the transformation
  does NOT assume the caller has anything GPU-resident, everything comes
  in as CPU and gets copied in at the head and back at the tail,
- promotes every transient Array to ``GPU_Global`` and every Scalar to
  ``Register``,
- assigns ``GPU_Device`` to top-level maps/libnodes, ``GPU_Default`` to
  top-level nested SDFGs, ``Sequential`` to everything below,
- propagates GPU storage into NSDFG connector bindings.

**Signature freeze.** The outer (non-transient) argument list of the
SDFG is snapshotted before ``OffloadVelocityToGPU`` runs and asserted
unchanged after. Any pass step that would flip an arg's type/shape
(e.g. a length-1 Array → Scalar rewrite) is rejected here -- stage 4
must leave the ABI that stage 3 produced intact so existing callers
link without a rebuild.

No ``offload_cpu``, no ``pre_gpu_fix``, no ``move_lib_schedules`` --
each of those was redundant given the structural contract stages 1-3
already produce; ``ToGPU`` + ``GPUKernelLaunchRestructure`` cover the
schedule + storage transitions on their own.

Run:
    python -m utils.stages.stage4              # optimise + compile (GPU)
    python -m utils.stages.stage4 --optimize   # codegen stage4 .sdfgz only
    python -m utils.stages.stage4 --compile    # compile existing stage4 sdfgz
"""

import argparse
import os
from pathlib import Path

# DaCe's CUDA codegen emits ``cudaDeviceSynchronize`` /
# ``cudaStreamSynchronize`` / ``cudaEventSynchronize`` calls whose
# placement is currently unreliable for velocity (the new GPU codegen
# rewrite addresses this; until then we run without emission). The
# ``__DACE_NO_SYNC`` env var in ``dace.codegen.common.no_sync_emission``
# gates every sync emission site. Default to enabled for this stage;
# callers can set ``__DACE_NO_SYNC=0`` explicitly to re-enable sync
# emission if needed.
os.environ.setdefault("__DACE_NO_SYNC", "1")

from utils.dace_branch import YAKUP_DEV_BRANCH, ensure_branch
ensure_branch(YAKUP_DEV_BRANCH)

import dace

from utils.passes.fuse_full_and_endpoint import fuse_full_and_endpoint
from utils.passes.offload_velocity_to_gpu import OffloadVelocityToGPU
from utils.passes.replace_reductions_with_tasklets import (
    replace_reductions_with_tasklets,
)
from utils.stages import common


STAGE_ID = 4


# Arrays the caller insists on keeping CPU-side. Host-only already
# skips offloading by structure, but list them explicitly -- the
# Fortran caller reads these from the host after the SDFG returns, so
# a stray copy-in/copy-out would sever the round-trip.
_KEEP_CPU = (
    # max_vcfl_dyn: scalar-shaped output Fortran reads on the host
    # after the SDFG returns. it is an array.
    "__CG_p_diag__m_max_vcfl_dyn",
    # ICON patch index/block descriptor arrays. These are read
    # exclusively by host-side interstate edges (to compute
    # ``i_startidx`` / ``i_endidx`` / ``i_startblk`` / ``i_endblk``
    # scalars). They are constants from the kernel's point of view;
    # the host-side interstate assignment requires them on CPU_Heap.
    # Adding gpu_ mirrors here would cause DaCe's validator to reject
    # the interstate edge as "inaccessible data container ... in host
    # code interstate edge".
    "__CG_p_patch__CG_cells__m_start_index",
    "__CG_p_patch__CG_cells__m_end_index",
    "__CG_p_patch__CG_cells__m_start_block",
    "__CG_p_patch__CG_cells__m_end_block",
    "__CG_p_patch__CG_edges__m_start_index",
    "__CG_p_patch__CG_edges__m_end_index",
    "__CG_p_patch__CG_edges__m_start_block",
    "__CG_p_patch__CG_edges__m_end_block",
    "__CG_p_patch__CG_verts__m_start_index",
    "__CG_p_patch__CG_verts__m_end_index",
    "__CG_p_patch__CG_verts__m_start_block",
    "__CG_p_patch__CG_verts__m_end_block",
)


def _signature_fingerprint(sdfg: dace.SDFG):
    """A hashable summary of the outer SDFG's argument list -- every
    non-transient entry in ``sdfg.arrays`` plus each free symbol.
    Compared across the pass to detect ABI drift."""
    args = []
    for name, desc in sdfg.arrays.items():
        if desc.transient:
            continue
        args.append((name,
                     type(desc).__name__,
                     str(desc.dtype),
                     tuple(str(d) for d in desc.shape)))
    symbols = [(s, str(t)) for s, t in sdfg.symbols.items()]
    args.sort()
    symbols.sort()
    return tuple(args), tuple(symbols)


def optimization_action(sdfg: dace.SDFG) -> dace.SDFG:
    # Snapshot the outer (caller-visible) signature before any pass
    # step runs; we assert it unchanged after OffloadVelocityToGPU so
    # downstream callers (e.g. main.cpp) keep linking without a
    # rebuild. Any type/shape drift surfaces here loudly.
    before = _signature_fingerprint(sdfg)

    OffloadVelocityToGPU(sdfg, exclude_from_offload=_KEEP_CPU)

    after = _signature_fingerprint(sdfg)
    if before != after:
        diff = _describe_signature_diff(before, after)
        raise RuntimeError(
            f"Stage #{STAGE_ID}: OffloadVelocityToGPU changed the outer "
            f"SDFG signature (ABI break). Diff:\n{diff}")

    # Replace every ``dace.libraries.standard.Reduce`` with a Tasklet
    # that calls our hand-written ``reduce_<op>_{cpu,gpu,device}``
    # helpers (``include/reductions_*.{h,cuh}``). Runs AFTER
    # OffloadVelocityToGPU so map schedules / array storages are
    # final and the per-Reduce backend pick is stable. Removes DaCe's
    # default segmented-reduction codegen path -- which emits
    # ``__gbar.Sync()`` calls that don't link in this build.
    replaced = replace_reductions_with_tasklets(sdfg)
    if replaced:
        print(f"Stage #{STAGE_ID}: replace_reductions_with_tasklets "
              f"rewrote {replaced} Reduce node(s)")

    # Watchtower: if the segmented-reduction workaround was ever
    # secretly needed, an ``out_val_0`` reference would leak into the
    # post-GPU SDFG. Stage 1's ``remove_clip_count`` + no `reduce_scan`
    # libnode means this should always be empty. Loud if not.
    leaks = _find_out_val_0(sdfg)
    if leaks:
        raise RuntimeError(
            f"Stage #{STAGE_ID}: {len(leaks)} unexpected 'out_val_0' "
            f"reference(s) survived OffloadVelocityToGPU: "
            f"{sorted(leaks)[:5]} ... -- revisit the dropped "
            f"pre_gpu_fixes.step_1 (segmented reduction pipelining).")

    # Promote every transient with ``AllocationLifetime.SDFG`` to
    # ``Persistent`` so its allocation lives across timesteps. Velocity
    # is invoked once per timestep from a long-lived host loop;
    # ``SDFG`` lifetime would re-alloc per call, ``Persistent`` keeps
    # the buffers warm and skips the malloc traffic.
    #promoted = _promote_transients_persistent(sdfg)
    promoted = False
    if promoted:
        print(f"Stage #{STAGE_ID}: promoted {promoted} transient(s) to "
              f"AllocationLifetime.Persistent")

    # Fuse a full-range map followed by an endpoint-zero map into a
    # single map with a ConditionalBlock body (see
    # ``fuse_full_and_endpoint`` docstring for the exact pattern --
    # ``z_w_con_c`` and similar arrays). Runs after the GPU schedule
    # is set so the fused map inherits the same ``GPU_Device``
    # schedule its halves had; merging two states into one also
    # removes a cross-state dependency that DaCe's CUDA codegen would
    # otherwise treat as a grid-barrier trigger.
    fused = fuse_full_and_endpoint(sdfg)
    if fused:
        print(f"Stage #{STAGE_ID}: fuse_full_and_endpoint merged "
              f"{fused} (full, endpoint-zero) state pair(s)")
        sdfg.validate()

    # Host-side body timer. Start immediately after the copy-in sync,
    # stop just before copy-out. Lives at stage 4 so the
    # OffloadVelocityToGPU output already carries the instrumentation
    # downstream stages inherit.
    _insert_body_timer(sdfg)

    sdfg.validate()
    return sdfg


def _promote_transients_persistent(sdfg: dace.SDFG) -> int:
    """Flip every transient array in ``sdfg`` (root + nested) on
    ``AllocationLifetime.SDFG`` over to ``Persistent``. Skips
    ``StorageType.Register`` -- those are stack-local in the
    generated function body and can't outlive a single call. Returns
    the count flipped."""
    from dace.sdfg import nodes as _nodes
    sdfgs = [sdfg]
    for n, _ in sdfg.all_nodes_recursive():
        if isinstance(n, _nodes.NestedSDFG):
            sdfgs.append(n.sdfg)
    count = 0
    for g in sdfgs:
        for desc in g.arrays.values():
            if not getattr(desc, 'transient', False):
                continue
            if desc.lifetime != dace.AllocationLifetime.SDFG:
                continue
            if desc.storage == dace.StorageType.Register:
                continue
            desc.lifetime = dace.AllocationLifetime.Persistent
            count += 1
    return count


def _insert_body_timer(sdfg: dace.SDFG) -> None:
    """Wrap the body with two host-side timer tasklets. Start state is
    inserted as a successor of ``_sync_after_copy_in``; stop state as
    a predecessor of ``_gpu_to_cpu_copy_out``. Reduction-induced sync
    inside the body suffices for accurate timing because the
    velocity body always contains a scalar-return reduction whose
    deferred sync flushes stream 0 before timer stop.
    """
    from dace.properties import CodeBlock as _CodeBlock
    existing = sdfg.global_code.get('frame')
    prior = existing.code if existing is not None \
        and isinstance(existing.code, str) else ''
    if '_stage4_t0' not in prior:
        sdfg.global_code['frame'] = _CodeBlock(
            prior
            + '#include <chrono>\n'
              '#include <cstdio>\n'
              'static std::chrono::steady_clock::time_point _stage4_t0;\n',
            dace.dtypes.Language.CPP)

    sync_in = _find_state_by_label(sdfg, '_sync_after_copy_in') \
        or sdfg.start_state
    copy_out = _find_state_by_label(sdfg, '_gpu_to_cpu_copy_out')
    if copy_out is None:
        ends = [s for s in sdfg.nodes() if sdfg.out_degree(s) == 0]
        copy_out = ends[-1] if ends else None

    start_state = sdfg.add_state_after(
        sync_in, label='_stage4_timer_start')
    start_state.add_tasklet(
        name='_stage4_timer_start',
        inputs={}, outputs={},
        code='_stage4_t0 = std::chrono::steady_clock::now();',
        language=dace.dtypes.Language.CPP,
        side_effects=True,
    )
    if copy_out is not None:
        stop_state = sdfg.add_state_before(
            copy_out, label='_stage4_timer_stop')
        stop_state.add_tasklet(
            name='_stage4_timer_stop',
            inputs={}, outputs={},
            code=(
                'auto _stage4_t1 = std::chrono::steady_clock::now();\n'
                'std::chrono::duration<double, std::milli> _stage4_dt = '
                '_stage4_t1 - _stage4_t0;\n'
                'std::printf("velocity body: %.6f ms\\n", _stage4_dt.count());'
            ),
            language=dace.dtypes.Language.CPP,
            side_effects=True,
        )


def _find_state_by_label(sdfg: dace.SDFG, label: str):
    for block in sdfg.nodes():
        if isinstance(block, dace.SDFGState) and block.label == label:
            return block
    return None


def _describe_signature_diff(before, after):
    before_args, before_syms = before
    after_args, after_syms = after
    removed = set(before_args) - set(after_args)
    added = set(after_args) - set(before_args)
    lines = []
    for r in sorted(removed):
        lines.append(f"  - REMOVED: {r}")
    for a in sorted(added):
        lines.append(f"  + ADDED:   {a}")
    if before_syms != after_syms:
        lines.append(f"  ! symbols: before={before_syms} after={after_syms}")
    return "\n".join(lines) or "  (no diff -- tuples unequal but elements match?)"


def _find_out_val_0(sdfg: dace.SDFG):
    hits = set()
    for name in sdfg.arrays:
        if 'out_val_0' in name:
            hits.add(('array', name))
    for n, _ in sdfg.all_nodes_recursive():
        for attr in ('data', 'label'):
            v = getattr(n, attr, None)
            if isinstance(v, str) and 'out_val_0' in v:
                hits.add(('node', v))
    for e, _ in sdfg.all_edges_recursive():
        if e.data is not None and getattr(e.data, 'data', None):
            if 'out_val_0' in e.data.data:
                hits.add(('memlet', e.data.data))
    return hits


def main():
    argp = argparse.ArgumentParser()
    argp.add_argument("--optimize", action=argparse.BooleanOptionalAction, default=False)
    argp.add_argument("--compile", action=argparse.BooleanOptionalAction, default=False)
    # Debug is the default. Debug means -O0 + DACE_VELOCITY_DEBUG +
    # IEEE-compliant fp (``--fmad=false``, ``--prec-div=true``,
    # ``--prec-sqrt=true``, ``--ftz=false``), which matches the CPU
    # reference bit-for-bit. Opt into release with ``--release`` for
    # -O3 + ``--use_fast_math`` (faster but FMA fusion can diverge
    # from CPU output by a few ULPs).
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
        # Velocity-owned reduction helpers.
        extra_sources = ["src/reductions.cpp", "src/reductions_kernel.cu"]
        extra_include_dirs = ["include"]
        common.compile_action(STAGE_ID, sdfgs, gpu=True, release=args.release,
                              extra_sources=extra_sources,
                              extra_include_dirs=extra_include_dirs)


if __name__ == "__main__":
    main()
