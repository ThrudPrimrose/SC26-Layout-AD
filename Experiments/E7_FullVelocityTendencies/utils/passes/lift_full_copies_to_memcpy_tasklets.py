"""Lift implicit full-extent CPU<->GPU copies into ``cudaMemcpyAsync`` tasklets.

Stage 4's ``OffloadVelocityToGPU`` emits the host<->device transfers as
direct ``AccessNode -> AccessNode`` edges in ``_cpu_to_gpu_copy_in`` /
``_gpu_to_cpu_copy_out`` states -- the natural representation of an
implicit cudaMemcpy. ``PermuteDimensions``'s ``require_core_dialect``
gate refuses any SDFG that still has implicit AN->AN edges, even ones
unrelated to the array being permuted.

This pass replaces each such full-extent cross-storage copy with::

    src_AN  ->  Tasklet[cudaMemcpyAsync(dst, src, n_bytes, kind, stream0)]  ->  dst_AN

The runtime semantics are IDENTICAL to the implicit copy (same
cudaMemcpyAsync call, same stream, same byte count). The only thing
that changes is the structural representation: now the copy is an
explicit GPU-scheduled tasklet, so the core-dialect gate sees no
implicit AN->AN edges and lets ``PermuteDimensions`` proceed. The
tasklet is scheduled ``GPU_Device`` (no ``side_effects``) so DaCe's
codegen does NOT bracket it with host-side stream syncs.

The emission style mirrors ``ExpandCUDAHostToDevice`` /
``ExpandCUDADeviceToDevice`` on the ``new-gpu-codegen-dev`` branch
(``dace/libraries/standard/nodes/copy_node.py``); that branch's
``CopyLibraryNode`` is the long-term home for this representation, but
its expansion infrastructure isn't on yakup/dev, so this pass just
emits the equivalent tasklet directly.
"""
import dace
from dace import dtypes
from dace.codegen.common import sym2cpp
from dace.memlet import Memlet
from dace.sdfg import nodes


# (src_is_gpu, dst_is_gpu) -> direction substring
_DIRECTION = {
    (False, True): "HostToDevice",
    (True, False): "DeviceToHost",
    (True, True): "DeviceToDevice",
}

_GPU_STORAGES = {
    dtypes.StorageType.GPU_Global,
    dtypes.StorageType.GPU_Shared,
}


def _is_gpu(storage):
    return storage in _GPU_STORAGES


def _is_full_extent(memlet, src_arr) -> bool:
    """True iff the memlet covers the full source extent."""
    if memlet is None or memlet.subset is None:
        return False
    full_volume = 1
    for s in src_arr.shape:
        full_volume = full_volume * s
    try:
        return memlet.subset.num_elements() == full_volume
    except Exception:
        return False


def lift_full_copies_to_memcpy_tasklets(sdfg: dace.SDFG) -> int:
    """Convert every full-extent CPU<->GPU AccessNode -> AccessNode edge
    into ``src -> Tasklet (cudaMemcpyAsync) -> dst``.

    Returns the number of edges converted. Walks the top-level SDFG and
    every nested SDFG inside it.
    """
    converted = 0
    sdfgs = [sdfg]
    for n, _ in sdfg.all_nodes_recursive():
        if isinstance(n, nodes.NestedSDFG):
            sdfgs.append(n.sdfg)
    for g in sdfgs:
        for state in list(g.all_states()):
            for edge in list(state.edges()):
                if not isinstance(edge.src, nodes.AccessNode):
                    continue
                if not isinstance(edge.dst, nodes.AccessNode):
                    continue
                src_arr = g.arrays.get(edge.src.data)
                dst_arr = g.arrays.get(edge.dst.data)
                if src_arr is None or dst_arr is None:
                    continue
                src_gpu = _is_gpu(src_arr.storage)
                dst_gpu = _is_gpu(dst_arr.storage)
                if not (src_gpu or dst_gpu):
                    continue  # CPU<->CPU edge: not our concern
                direction = _DIRECTION.get((src_gpu, dst_gpu))
                if direction is None:
                    continue
                if not _is_full_extent(edge.data, src_arr):
                    continue
                _replace_with_memcpy_tasklet(state, edge, direction, src_arr, dst_arr)
                converted += 1
    return converted


def _replace_with_memcpy_tasklet(state, edge, direction: str,
                                 src_arr, dst_arr) -> None:
    """``src_AN -> dst_AN``  ==>  ``src_AN -> tasklet -> dst_AN``."""
    src_node, dst_node = edge.src, edge.dst

    n_elements = 1
    for s in src_arr.shape:
        n_elements = n_elements * s

    code = (
        f"cudaMemcpyAsync(_memcpy_out, _memcpy_in, "
        f"{sym2cpp(n_elements)} * sizeof({src_arr.dtype.ctype}), "
        f"cudaMemcpy{direction}, "
        f"__state->gpu_context->streams[0]);"
    )

    tasklet = state.add_tasklet(
        name=f"memcpy_{src_node.data}_to_{dst_node.data}",
        inputs={"_memcpy_in"},
        outputs={"_memcpy_out"},
        code=code,
        language=dtypes.Language.CPP,
    )
    # Pointer-typed connectors so codegen passes raw pointers (not
    # dereferenced scalars) into the tasklet body. Always safe for full
    # multi-element extents -- the single-element CPU dance from the
    # CopyLibraryNode expansion isn't needed here because we only fire
    # on full-extent copies (multi-element by construction).
    tasklet.in_connectors["_memcpy_in"] = dtypes.pointer(src_arr.dtype)
    tasklet.out_connectors["_memcpy_out"] = dtypes.pointer(dst_arr.dtype)
    # Schedule as GPU_Device so DaCe doesn't bracket the tasklet with
    # host-side ``cudaStreamSynchronize`` calls (which it does for any
    # CPU-scheduled tasklet that touches GPU memory). The cudaMemcpy
    # itself is the GPU-side op; no surrounding host sync needed.
    tasklet.schedule = dtypes.ScheduleType.GPU_Device

    state.add_edge(
        src_node, None, tasklet, "_memcpy_in",
        Memlet.from_array(src_node.data, src_arr),
    )
    state.add_edge(
        tasklet, "_memcpy_out", dst_node, None,
        Memlet.from_array(dst_node.data, dst_arr),
    )
    state.remove_edge(edge)
