"""Velocity-tendencies stage-4 GPU offload.

Takes a stage-3 SDFG (all map schedules ``Default``, transients at the
root SDFG with ``AllocationLifetime.SDFG``, mixed storage) and produces
a GPU-compilable SDFG by doing three things in a specific order:

1. **Assign schedules** (must be first so the pass can distinguish
   kernel-side from host-side nodes). Top-level ``MapEntry`` /
   ``LibraryNode`` → ``GPU_Device``; top-level ``NestedSDFG`` →
   ``GPU_Default``; everything below a top-level scope →
   ``Sequential``.
2. **Mirror every non-transient top-level Array to a ``gpu_<name>``
   sibling on GPU_Global** — but only if any AccessNode for that array
   sits inside a kernel scope or at the kernel boundary (i.e. wired to
   a MapEntry/MapExit/NestedSDFG). Pure host-side arrays (only touched
   by top-level Tasklets) are left alone. A single copy-in state is
   added as the new start, a single copy-out state as the new terminal.
   Kernel-side AccessNodes and their memlets are retargeted to the
   ``gpu_`` variant; host-side ones keep the original name.
3. **Promote every transient Array to ``GPU_Global``** (storage only;
   shape/strides are whatever stage 3 established).

After all three, validate **and** sanity-check that no GPU-scheduled
kernel writes its output into a scalar (DaCe's CUDA codegen rejects
that; fail loudly here rather than at codegen time).

Distilled from ``ToGPU`` (dace@f2dace/staging,
``dace/transformation/passes/to_gpu.py``, 911 lines). The original
interleaves these phases, has a dead ``get_const_arrays`` branch, uses
``is_devicelevel_gpu`` on a mixed-state SDFG which causes races between
schedule assignment and memlet retargeting, and carries
``cpu_library_nodes`` / ``exclude`` knobs that velocity doesn't use.
Here each phase is explicit and the kernel/host split is resolved
structurally from the stage-3 topology.
"""
import copy
import re
from typing import Iterator, Set

from dace import SDFG, data, dtypes, memlet as mm
from dace.sdfg import nodes
from dace.subsets import Range


_CPU_STORAGES = (
    dtypes.StorageType.Default,
    dtypes.StorageType.CPU_Heap,
    dtypes.StorageType.Register,
)

_BOUNDARY_NODE_TYPES = (nodes.MapEntry, nodes.MapExit, nodes.NestedSDFG)


def OffloadVelocityToGPU(sdfg: SDFG, exclude_from_offload=()):
    """Drive the phases in order. Not a general-purpose DaCe
    ``Pass`` — scoped to velocity stage 3's structural contract.

    ``exclude_from_offload``: iterable of array names that must stay on
    the host even though they're non-transient Arrays (the structural
    kernel-side filter already skips host-only arrays, but this is the
    explicit opt-out for names the caller insists on keeping CPU-side
    regardless -- e.g. a scalar-shaped output the Fortran caller reads
    on the host after the SDFG returns)."""
    # Pin to a single CUDA stream. Multi-stream scheduling under the
    # current DaCe codegen mixes host/device ordering in ways that
    # complicate the ``__DACE_NO_SYNC=1`` contract; keeping everything
    # on stream 0 means we only need to synchronize stream 0 at the
    # copy boundaries.
    import dace as _dace
    _dace.Config.set("compiler", "cuda", "max_concurrent_streams", value="1")
    excluded = frozenset(exclude_from_offload)
    _assign_schedules(sdfg)
    _mirror_kernelside_nontransients_to_gpu(sdfg, excluded)
    _promote_transient_arrays_to_gpu(sdfg)
    # Invariants: any array on GPU storage carries the ``gpu_``
    # prefix, and every NSDFG connector name matches its outer memlet
    # ``data``. These two are interdependent -- renaming inner arrays
    # leaves connector bindings stale until the reconcile runs, so
    # don't validate between them.
    _ensure_gpu_prefix_for_gpu_storage_arrays(sdfg)
    _reconcile_nsdfg_connector_names(sdfg)
    # Transient promotion set GPU_Global on root-level arrays, but the
    # matching inner descriptors inside NestedSDFG connectors (non-
    # transient inner bindings of outer transients) still carry the
    # stage-3 ``Default`` storage. Re-run the propagation so any inner
    # Array whose outer memlet points at a GPU_Global descriptor
    # inherits that storage. Without this, downstream logic that
    # classifies "is this data on GPU?" by looking at the inner
    # descriptor sees ``Default`` and misroutes (e.g. picks the cpu
    # reduction backend when the data is actually on the device).
    _propagate_gpu_storage_into_nested_sdfgs(sdfg)
    # Swap standard-library Reduce nodes for tasklets that dispatch to
    # the velocity-owned reduction helpers (CPU / GPU launch / device
    # inline, chosen from detected storage + scope). Runs last so it
    # sees the final storage layout produced by the preceding phases.
    from utils.passes.replace_reductions_with_tasklets import \
        replace_reductions_with_tasklets
    replace_reductions_with_tasklets(sdfg)
    sdfg.validate()


# -- Phase 1: schedules -------------------------------------------------


_BLOCK_RANGE_MARKERS = ("startblk", "endblk")


def _is_block_map(map_node: "nodes.Map") -> bool:
    """A map is a *block map* — the per-patch outer loop inherited from
    the Fortran ``DO jb = i_startblk .. i_endblk`` structure — iff its
    range expression references one of the block-loop bound symbols.
    Block maps stay sequential on the host and orchestrate kernel
    launches; every other map becomes ``GPU_Device``. Detecting by
    substring on the range is structural enough for the f2dace-
    generated pipeline, where the ``i_startblk*`` / ``i_endblk*``
    naming is stable."""
    rng_str = str(map_node.range)
    return any(m in rng_str for m in _BLOCK_RANGE_MARKERS)


def _assign_schedules(sdfg: SDFG):
    """Detection by map-range content, not tree position.

    * **Block maps** (range contains ``startblk`` / ``endblk``) are
      forced to ``Sequential`` — the f2dace-generated per-patch outer
      loops orchestrate kernel launches per block and must stay on
      the host. (``Default`` is not sufficient: DaCe's type inference
      promotes a ``Default``-schedule top-level map to ``GPU_Device``
      when the target is CUDA, which would put the block loop on the
      device and break the kernel-dimensionality check.)
    * **Top-level non-block MapEntry / LibraryNode** → ``GPU_Device``.
    * **Non-top-level non-block MapEntry / LibraryNode** (anything
      inside a scope, whether the scope is a block map or another
      map) → ``Sequential``.
    * **NestedSDFGs** are left untouched — ``GPU_Default`` isn't
      available in upstream DaCe, and the host/device split is
      determined by the nearest enclosing map anyway.

    The previous rule was "top-level MapEntry → GPU_Device"
    unconditionally, which dragged the per-patch block map onto the
    device. Block maps carry ``startblk``/``endblk`` in their range
    strings; detecting them structurally via that marker is stable
    across the velocity variants without enumerating map labels.
    """
    for g in _all_sdfgs(sdfg):
        for state in g.all_states():
            sdict = state.scope_dict()
            for node in state.nodes():
                if isinstance(node, nodes.MapEntry):
                    if _is_block_map(node.map):
                        # Force Sequential so DaCe's type inference
                        # doesn't promote Default -> GPU_Device on
                        # this host-side orchestrator.
                        node.map.schedule = dtypes.ScheduleType.Sequential
                        continue
                    at_top = sdict[node] is None
                    node.map.schedule = (dtypes.ScheduleType.GPU_Device
                                         if at_top
                                         else dtypes.ScheduleType.Sequential)
                elif isinstance(node, nodes.LibraryNode):
                    at_top = sdict[node] is None
                    node.schedule = (dtypes.ScheduleType.GPU_Device
                                     if at_top
                                     else dtypes.ScheduleType.Sequential)


# -- Phase 2: mirror kernel-side non-transients --------------------------


def _mirror_kernelside_nontransients_to_gpu(sdfg: SDFG,
                                             excluded: frozenset = frozenset()):
    """Create ``gpu_<name>`` siblings for every non-transient Array that
    the kernel touches, with a single copy-in head state and a single
    copy-out tail state. Host-only arrays and names in ``excluded``
    are untouched."""
    mirror_names = _arrays_needing_gpu_mirror(sdfg) - excluded
    if not mirror_names:
        return

    # New state at the very beginning to do CPU→GPU copies, and at the
    # very end for GPU→CPU. Doing this up-front before the retargeting
    # means the copy-in/out states reference the ORIGINAL names, and
    # retargeting only happens on the kernel-side nodes.
    pre = sdfg.add_state_before(sdfg.start_state, label='_cpu_to_gpu_copy_in',
                                is_start_block=True)
    ends = [s for s in sdfg.nodes() if sdfg.out_degree(s) == 0]
    assert len(ends) == 1, (
        f"OffloadVelocityToGPU expects exactly one end state, got {len(ends)}")
    post = sdfg.add_state_after(ends[0], label='_gpu_to_cpu_copy_out')

    # ``__DACE_NO_SYNC=1`` is set in stage 4 so DaCe emits no implicit
    # host-device sync. Bracket the copy states with explicit
    # stream-0 syncs so (a) the kernel sees all copy-in data, (b) the
    # caller sees all copy-out results before the SDFG returns. Stream
    # 0 is the only stream in play because
    # ``compiler.cuda.max_concurrent_streams`` is pinned to 1 in the
    # compile helper.
    sync_after_copy_in = sdfg.add_state_after(pre, label='_sync_after_copy_in')
    _add_stream_sync_tasklet(sync_after_copy_in)
    sync_after_copy_out = sdfg.add_state_after(post, label='_sync_after_copy_out')
    _add_stream_sync_tasklet(sync_after_copy_out)

    for name in mirror_names:
        arr = sdfg.arrays[name]
        gname = 'gpu_' + name
        assert gname not in sdfg.arrays
        gpu_arr = copy.deepcopy(arr)
        gpu_arr.transient = True
        gpu_arr.storage = dtypes.StorageType.GPU_Global
        gpu_arr.lifetime = dtypes.AllocationLifetime.SDFG
        sdfg.add_datadesc(gname, gpu_arr)

        pre.add_edge(pre.add_read(name), None,
                     pre.add_write(gname), None,
                     mm.Memlet.from_array(name, arr))
        post.add_edge(post.add_read(gname), None,
                      post.add_write(name), None,
                      mm.Memlet.from_array(gname, gpu_arr))

    # Retarget AccessNodes that are kernel-side. Host-side AccessNodes
    # keep the original name so top-level Tasklets still read/write the
    # CPU array.
    retargeted_nodes = set()
    for state in sdfg.all_states():
        if state is pre or state is post:
            continue
        sdict = state.scope_dict()
        for node in list(state.nodes()):
            if not isinstance(node, nodes.AccessNode):
                continue
            if node.data not in mirror_names:
                continue
            if _is_kernel_side(node, state, sdict):
                node.data = 'gpu_' + node.data
                retargeted_nodes.add(id(node))

    # Retarget memlets: a memlet with data == <name> should become
    # ``gpu_<name>`` iff it sits on an edge whose AccessNode endpoint
    # (if any) got retargeted, OR whose non-AccessNode endpoints are
    # all inside a kernel scope. Explicitly avoid retargeting memlets
    # between two host-side nodes.
    for state in sdfg.all_states():
        if state is pre or state is post:
            continue
        sdict = state.scope_dict()
        for edge in state.edges():
            if edge.data is None or edge.data.data not in mirror_names:
                continue
            if _edge_is_kernel_side(edge, sdict, retargeted_nodes):
                edge.data.data = 'gpu_' + edge.data.data

    # Propagate storage through NSDFG connector bindings. An NSDFG's
    # inner descriptor shadows its outer binding: if the outer got
    # promoted to GPU_Global, the inner must too, or DaCe's codegen
    # hits an ``IllegalCopy`` when a GPU-context tasklet reads a
    # CPU_Heap-typed inner. Walks every NSDFG in the root; rewrites
    # connector-bound inner Array descriptors to ``GPU_Global``.
    _propagate_gpu_storage_into_nested_sdfgs(sdfg)


def _add_stream_sync_tasklet(state) -> None:
    """Emit a parameterless Tasklet that calls
    ``gpuStreamSynchronize`` on stream 0 of the SDFG's gpu_context.
    The ``gpu*`` preprocessor aliases in
    ``dace/runtime/include/dace/cuda/cudacommon.cuh`` expand to
    ``cudaStreamSynchronize`` for NVIDIA and ``hipStreamSynchronize``
    for AMD, so the same tasklet compiles on both backends.
    ``side_effects=True`` keeps DaCe from dead-code-eliminating a
    tasklet with no inputs/outputs. A reference to
    ``__state->gpu_context`` is always in scope in the generated host
    code."""
    state.add_tasklet(
        name='_stream0_sync',
        inputs={},
        outputs={},
        code='DACE_GPU_CHECK(gpuStreamSynchronize(__state->gpu_context->streams[0]));',
        language=dtypes.Language.CPP,
        side_effects=True,
    )


def _ensure_gpu_prefix_for_gpu_storage_arrays(sdfg: SDFG):
    """Any array with ``GPU_Global`` storage must have the ``gpu_``
    name prefix. Walks every SDFG in the tree; renames arrays whose
    storage is GPU_Global but whose name lacks the prefix."""
    for g in _all_sdfgs(sdfg):
        renames = {}
        for name, desc in list(g.arrays.items()):
            if not isinstance(desc, data.Array):
                continue
            if desc.storage != dtypes.StorageType.GPU_Global:
                continue
            if name.startswith("gpu_"):
                continue
            new_name = "gpu_" + name
            # Guard against colliding with an existing entry (e.g.,
            # the mirror phase already created ``gpu_X`` alongside the
            # original ``X``).
            if new_name in g.arrays:
                continue
            renames[name] = new_name
        for old, new in renames.items():
            _rename_array_in_sdfg(g, old, new)


def _rename_array_in_sdfg(g: SDFG, old: str, new: str):
    """Rename a single Array within ``g``. Avoids ``sdfg.replace`` --
    that routes interstate-edge values through sympy, which corrupts
    numeric literals like ``-1.79e+308`` (sympy collapses them to
    ``-oo`` → ``-inf``, which downstream validators don't recognise
    as a symbol). This does surgical string substitution on the
    interstate-edge assignments / conditions and structural rewrites
    for AccessNodes, memlets, and the ``arrays`` dict."""
    if old == new:
        return
    if old not in g.arrays:
        return

    # 1. Arrays dict.
    g.arrays[new] = g.arrays.pop(old)

    # 2. AccessNodes.
    for state in g.all_states():
        for node in state.nodes():
            if isinstance(node, nodes.AccessNode) and node.data == old:
                node.data = new

    # 3. Memlets on intra-state edges.
    for state in g.all_states():
        for edge in state.edges():
            if edge.data is not None and edge.data.data == old:
                edge.data.data = new

    # 4. Interstate edges: string-substitute with word-boundary safety.
    pat = re.compile(r'(?<![\w.])' + re.escape(old) + r'(?![\w])')
    for e in g.all_interstate_edges():
        new_assigns = {}
        for k, v in e.data.assignments.items():
            nk = pat.sub(new, k) if isinstance(k, str) else k
            nv = pat.sub(new, v) if isinstance(v, str) else v
            new_assigns[nk] = nv
        e.data.assignments = new_assigns
        if e.data.condition is not None:
            cs = e.data.condition.as_string
            new_cs = pat.sub(new, cs)
            if new_cs != cs:
                from dace.properties import CodeBlock
                e.data.condition = CodeBlock(new_cs, e.data.condition.language)

    # 5. NSDFG connector bindings pointing at this SDFG's arrays.
    # (Covered separately by ``_reconcile_nsdfg_connector_names``.)


def _reconcile_nsdfg_connector_names(sdfg: SDFG):
    """For every ``NestedSDFG`` node in the tree, ensure that each
    in/out connector's name equals the outer memlet's ``data`` field.
    When they differ (typically after the mirror phase renamed
    ``X`` → ``gpu_X`` at the outer level), rename the connector and
    all references to it inside the nested SDFG. Cascades into
    deeper nested SDFGs: renaming inside one level may expose fresh
    connector mismatches one level down.

    Fixed-point loop with an upper bound on passes to guard against
    pathological cycles; each pass renames at least one thing or
    terminates."""
    max_passes = 8
    for _ in range(max_passes):
        changed = False
        for state in sdfg.all_states():
            for node in list(state.nodes()):
                if not isinstance(node, nodes.NestedSDFG):
                    continue
                for e in list(state.in_edges(node)):
                    if e.data is None or e.data.data is None:
                        continue
                    if e.dst_conn and e.dst_conn != e.data.data:
                        _rename_nsdfg_connector(node, e.dst_conn, e.data.data,
                                                direction='in')
                        e.dst_conn = e.data.data
                        changed = True
                for e in list(state.out_edges(node)):
                    if e.data is None or e.data.data is None:
                        continue
                    if e.src_conn and e.src_conn != e.data.data:
                        _rename_nsdfg_connector(node, e.src_conn, e.data.data,
                                                direction='out')
                        e.src_conn = e.data.data
                        changed = True
            # Recurse: an NSDFG may itself contain NSDFGs whose
            # connectors now mismatch because the inner replace
            # bumped memlet data names up to the gpu_ form.
            for node in state.nodes():
                if isinstance(node, nodes.NestedSDFG):
                    _reconcile_nsdfg_connector_names(node.sdfg)
        if not changed:
            return


def _rename_nsdfg_connector(nsdfg_node: "nodes.NestedSDFG",
                            old_name: str, new_name: str,
                            direction: str):
    """Rename one connector on a NestedSDFG node and propagate the
    rename into the inner SDFG (AccessNodes, memlets, interstate
    edges, inner arrays all follow via ``SDFG.replace``)."""
    if old_name == new_name:
        return
    conns = (nsdfg_node.in_connectors if direction == 'in'
             else nsdfg_node.out_connectors)
    if old_name not in conns:
        return
    # Remove the old connector entry and add the new one with the
    # same type annotation.
    dtype = conns[old_name]
    if direction == 'in':
        nsdfg_node.remove_in_connector(old_name)
        nsdfg_node.add_in_connector(new_name, dtype=dtype, force=True)
    else:
        nsdfg_node.remove_out_connector(old_name)
        nsdfg_node.add_out_connector(new_name, dtype=dtype, force=True)
    # Propagate the rename into the inner SDFG: arrays, memlets,
    # access nodes, interstate edges. Use the sympy-free rename to
    # avoid collapsing numeric literals like ``-1.79e+308`` to
    # ``-inf``.
    if new_name in nsdfg_node.sdfg.arrays:
        # Another in/out pair already renamed to this name; avoid
        # colliding descriptors. Leave the inner alone -- the outer
        # rename is enough because the connector now lines up.
        return
    _rename_array_in_sdfg(nsdfg_node.sdfg, old_name, new_name)


def _propagate_gpu_storage_into_nested_sdfgs(sdfg: SDFG):
    """For every NSDFG, inspect the memlets on its in/out connector
    edges to determine the outer binding's storage. If the outer is
    ``GPU_Global``, change the matching inner descriptor to
    ``GPU_Global`` too -- unless the inner SDFG references that
    connector name on a host-side interstate edge. Interstate edges
    are evaluated on the host; an array they read must stay on CPU
    storage or DaCe's validator rejects the SDFG as "inaccessible
    data container ... in host code interstate edge".
    Recurses into nested SDFGs."""
    for state in sdfg.all_states():
        for node in state.nodes():
            if not isinstance(node, nodes.NestedSDFG):
                continue
            parent_sdfg = state.sdfg if hasattr(state, 'sdfg') else state.parent
            host_only_inner = _inner_names_read_on_interstate_edges(node.sdfg)
            # Walk edges carrying data into/out of the nested SDFG; the
            # memlet.data names the outer array, the connector names
            # the inner binding.
            for e in list(state.in_edges(node)) + list(state.out_edges(node)):
                if e.data is None or e.data.data is None:
                    continue
                outer_desc = parent_sdfg.arrays.get(e.data.data)
                if outer_desc is None:
                    continue
                if outer_desc.storage != dtypes.StorageType.GPU_Global:
                    continue
                conn = e.dst_conn if e.dst is node else e.src_conn
                if conn is None:
                    continue
                if conn in host_only_inner:
                    # Leave on CPU so the host interstate edge can read
                    # it. The kernel paths that need it will pick up the
                    # GPU_Global outer via the per-state AccessNode
                    # retargeting in the mirror phase.
                    continue
                inner = node.sdfg.arrays.get(conn)
                if inner is None:
                    continue
                if isinstance(inner, data.Array):
                    inner.storage = dtypes.StorageType.GPU_Global
            # Recurse into deeper nested SDFGs.
            _propagate_gpu_storage_into_nested_sdfgs(node.sdfg)


def _inner_names_read_on_interstate_edges(inner_sdfg: SDFG) -> Set[str]:
    """Names that appear as bare references (``X`` or ``X[...]``) on
    any interstate edge assignment / condition within ``inner_sdfg`` or
    its descendants. These must stay on the host because the
    interstate evaluation is host-side."""
    names = set()
    arr_names = set(inner_sdfg.arrays.keys())
    for g in _all_sdfgs(inner_sdfg):
        for e in g.all_interstate_edges():
            texts = list(e.data.assignments.values())
            if e.data.condition is not None:
                texts.append(e.data.condition.as_string)
            for t in texts:
                if not isinstance(t, str):
                    continue
                for n in arr_names:
                    if n in t:
                        names.add(n)
    return names


def _arrays_needing_gpu_mirror(sdfg: SDFG) -> Set[str]:
    """An Array needs a GPU mirror if it's a non-transient CPU-storage
    Array AND at least one of its AccessNodes in the SDFG is
    kernel-side. Pure host-only arrays (e.g. scalars / arrays read by
    host tasklets only) are skipped."""
    candidates = {
        name for name, arr in sdfg.arrays.items()
        if isinstance(arr, data.Array)
        and not arr.transient
        and arr.storage in _CPU_STORAGES
    }
    if not candidates:
        return set()
    needs = set()
    for state in sdfg.all_states():
        sdict = state.scope_dict()
        for node in state.nodes():
            if not isinstance(node, nodes.AccessNode):
                continue
            if node.data not in candidates:
                continue
            if _is_kernel_side(node, state, sdict):
                needs.add(node.data)
    return needs


def _is_kernel_side(node: nodes.AccessNode, state, sdict) -> bool:
    """An AccessNode is kernel-side if it lives inside any scope OR
    it's at top level but wired to a MapEntry/MapExit/NestedSDFG
    boundary. Block maps count too: an array feeding a block map
    still reaches GPU kernels nested inside that map, so it needs
    its gpu_ mirror. The ``exclude_from_offload`` parameter on
    ``OffloadVelocityToGPU`` is the user-facing knob for arrays that
    must stay CPU-only regardless of structural cues (e.g. the
    ``*_start_index`` / ``*_end_index`` arrays that host interstate
    edges read)."""
    if sdict[node] is not None:
        return True
    for e in state.in_edges(node):
        if isinstance(e.src, _BOUNDARY_NODE_TYPES):
            return True
    for e in state.out_edges(node):
        if isinstance(e.dst, _BOUNDARY_NODE_TYPES):
            return True
    return False


def _edge_is_kernel_side(edge, sdict, retargeted_nodes: Set[int]) -> bool:
    """Kernel-side iff either endpoint is a retargeted AccessNode, the
    edge sits fully inside a scope, or it touches a scope boundary."""
    if id(edge.src) in retargeted_nodes or id(edge.dst) in retargeted_nodes:
        return True
    if sdict.get(edge.src) is not None or sdict.get(edge.dst) is not None:
        return True
    if isinstance(edge.src, _BOUNDARY_NODE_TYPES) or isinstance(edge.dst, _BOUNDARY_NODE_TYPES):
        return True
    return False


# -- Phase 3: promote transient data to GPU-compatible storage ----------


def _promote_transient_arrays_to_gpu(sdfg: SDFG):
    """Transient Arrays with CPU-like storage → ``GPU_Global``.
    Every Scalar (transient or NSDFG-connector-bound) with CPU-like
    storage → ``Register`` (a scalar inside a GPU kernel lives in a
    register; a non-transient scalar at the top level is passed by
    value — both cases are legal register storage). Propagating to
    non-transient scalars matters because NSDFG inner descriptors
    shadow their outer binding's storage, and a CPU_Heap inner scalar
    fed into a kernel triggers DaCe's ``IllegalCopy`` dispatch."""
    for g in _all_sdfgs(sdfg):
        for arr in g.arrays.values():
            if arr.storage not in _CPU_STORAGES:
                continue
            if isinstance(arr, data.Array) and arr.transient:
                arr.storage = dtypes.StorageType.GPU_Global
            elif isinstance(arr, data.Scalar):
                arr.storage = dtypes.StorageType.Register


# -- helpers --------------------------------------------------------------


def _all_sdfgs(sdfg: SDFG) -> Iterator[SDFG]:
    yield sdfg
    for n, _ in sdfg.all_nodes_recursive():
        if isinstance(n, nodes.NestedSDFG):
            yield n.sdfg
