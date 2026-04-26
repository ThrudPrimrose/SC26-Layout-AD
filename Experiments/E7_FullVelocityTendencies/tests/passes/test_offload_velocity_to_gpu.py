"""Tests for ``OffloadVelocityToGPU``.

Structural tests (purely in-process, no CUDA needed). Each test builds
a small SDFG that models one piece of the stage-3 contract, runs
``OffloadVelocityToGPU``, and asserts the expected post-conditions.

Numerical tests aren't included -- stage 4's output requires a GPU for
end-to-end validation, and the velocity pipeline uses
``got_and_want`` snapshots for that higher-level check. Here we cover
the pass's mutation invariants.
"""
import pytest

import dace
from dace import dtypes, memlet as mm, nodes
from dace.sdfg import SDFG

from utils.passes.offload_velocity_to_gpu import OffloadVelocityToGPU


def _inner_sdfg(M=4, N=4) -> SDFG:
    """Inner NSDFG: reads A, writes B[i, j] = 2 * A[i, j]."""
    inner = SDFG("inner")
    for name in ("A", "B"):
        inner.add_array(name, [M, N], dace.float64, transient=False)
    st = inner.add_state()
    ar, bw = st.add_read("A"), st.add_write("B")
    me, mx = st.add_map("k", {"i": f"0:{M}", "j": f"0:{N}"})
    t = st.add_tasklet("mul2", {"a"}, {"o"}, "o = 2.0 * a")
    me.add_in_connector("IN_A")
    me.add_out_connector("OUT_A")
    mx.add_in_connector("IN_B")
    mx.add_out_connector("OUT_B")
    st.add_edge(ar, None, me, "IN_A", mm.Memlet.from_array("A", inner.arrays["A"]))
    st.add_edge(me, "OUT_A", t, "a", mm.Memlet(data="A", subset="i, j"))
    st.add_edge(t, "o", mx, "IN_B", mm.Memlet(data="B", subset="i, j"))
    st.add_edge(mx, "OUT_B", bw, None, mm.Memlet.from_array("B", inner.arrays["B"]))
    return inner


def _stage3_shaped_sdfg(M=4, N=4, K=3) -> SDFG:
    """Minimal stage-3-shaped top SDFG: one block-parallel MapEntry
    containing a NestedSDFG that runs the inner work per block.
    Non-transient A, B at the top; one transient scratch ``t`` at the
    top with ``AllocationLifetime.SDFG`` (what stage 3 guarantees)."""
    top = SDFG("top_stage3")
    top.add_array("A", [M, N, K], dace.float64, transient=False)
    top.add_array("B", [M, N, K], dace.float64, transient=False)
    top.add_array("t", [M, N, K], dace.float64,
                  transient=True,
                  lifetime=dtypes.AllocationLifetime.SDFG)
    st = top.add_state()
    me, mx = st.add_map("blocks", {"jb": f"0:{K}"})
    me.add_in_connector("IN_A")
    me.add_out_connector("OUT_A")
    mx.add_in_connector("IN_B")
    mx.add_out_connector("OUT_B")
    n = st.add_nested_sdfg(_inner_sdfg(M=M, N=N), {"A"}, {"B"})
    ar = st.add_read("A")
    bw = st.add_write("B")
    st.add_edge(ar, None, me, "IN_A", mm.Memlet.from_array("A", top.arrays["A"]))
    st.add_edge(me, "OUT_A", n, "A", mm.Memlet(data="A", subset=f"0:{M}, 0:{N}, jb"))
    st.add_edge(n, "B", mx, "IN_B", mm.Memlet(data="B", subset=f"0:{M}, 0:{N}, jb"))
    st.add_edge(mx, "OUT_B", bw, None, mm.Memlet.from_array("B", top.arrays["B"]))
    return top


def test_schedules_assigned_gpu_by_range_not_position():
    """Non-block maps at any depth become ``GPU_Device``; block maps
    (range contains ``startblk``/``endblk``) become ``Sequential`` as
    host orchestrators. The helper SDFG here has no block markers, so
    every map -- outer and nested -- should be ``GPU_Device``."""
    top = _stage3_shaped_sdfg()
    OffloadVelocityToGPU(top)
    top_maps = [
        n for st in top.all_states()
        for n in st.nodes()
        if isinstance(n, nodes.MapEntry) and st.entry_node(n) is None
    ]
    assert len(top_maps) == 1
    assert top_maps[0].map.schedule == dtypes.ScheduleType.GPU_Device
    # Inner maps sit at the top of their own NSDFG -- they also
    # become GPU_Device under the new range-based rule.
    for n, _ in top.all_nodes_recursive():
        if isinstance(n, nodes.NestedSDFG):
            for st in n.sdfg.all_states():
                for inner in st.nodes():
                    if isinstance(inner, nodes.MapEntry):
                        assert inner.map.schedule == dtypes.ScheduleType.GPU_Device


def test_block_map_stays_sequential_when_range_has_startblk():
    """A map whose range contains ``startblk`` / ``endblk`` is the
    host orchestrator and must stay ``Sequential`` even though it's
    at top level. The per-block work lives in a NestedSDFG (matching
    the stage-3 contract); ``OffloadVelocityToGPU`` may freely turn
    the NestedSDFG's inner map into ``GPU_Device`` without touching
    the outer block map."""
    M, N, K = 4, 4, 3
    top = SDFG("with_block_map")
    top.add_array("A", [M, N, K], dace.float64, transient=False)
    top.add_array("B", [M, N, K], dace.float64, transient=False)
    top.add_symbol("i_startblk", dace.int32)
    top.add_symbol("i_endblk", dace.int32)
    st = top.add_state()
    me, mx = st.add_map("blocks", {"jb": "i_startblk : i_endblk"})
    me.add_in_connector("IN_A")
    me.add_out_connector("OUT_A")
    mx.add_in_connector("IN_B")
    mx.add_out_connector("OUT_B")
    n = st.add_nested_sdfg(_inner_sdfg(M=M, N=N), {"A"}, {"B"})
    ar, bw = st.add_read("A"), st.add_write("B")
    st.add_edge(ar, None, me, "IN_A", mm.Memlet.from_array("A", top.arrays["A"]))
    st.add_edge(me, "OUT_A", n, "A", mm.Memlet(data="A", subset=f"0:{M}, 0:{N}, jb"))
    st.add_edge(n, "B", mx, "IN_B", mm.Memlet(data="B", subset=f"0:{M}, 0:{N}, jb"))
    st.add_edge(mx, "OUT_B", bw, None, mm.Memlet.from_array("B", top.arrays["B"]))

    OffloadVelocityToGPU(top)

    assert me.map.schedule == dtypes.ScheduleType.Sequential


def test_nontransient_arrays_get_gpu_mirror_and_copy_states():
    top = _stage3_shaped_sdfg()
    OffloadVelocityToGPU(top)

    # GPU siblings were created for both non-transient kernel-side arrays.
    assert "gpu_A" in top.arrays
    assert "gpu_B" in top.arrays
    assert top.arrays["gpu_A"].storage == dtypes.StorageType.GPU_Global
    assert top.arrays["gpu_A"].transient is True
    assert top.arrays["gpu_B"].transient is True

    # Original non-transients remain CPU-side.
    assert top.arrays["A"].transient is False
    assert top.arrays["B"].transient is False

    # Two bracketing copy states exist, each with at least one edge.
    labels = {s.label for s in top.states()}
    assert "_cpu_to_gpu_copy_in" in labels
    assert "_gpu_to_cpu_copy_out" in labels


def test_transient_arrays_promoted_and_gpu_prefixed():
    """Transients move to ``GPU_Global`` AND get the ``gpu_`` prefix
    (enforced by ``_ensure_gpu_prefix_for_gpu_storage_arrays``)."""
    top = _stage3_shaped_sdfg()
    OffloadVelocityToGPU(top)
    # Original ``t`` has been renamed.
    assert "t" not in top.arrays
    assert "gpu_t" in top.arrays
    assert top.arrays["gpu_t"].storage == dtypes.StorageType.GPU_Global
    assert top.arrays["gpu_t"].transient is True


def test_host_only_nontransient_is_left_on_cpu():
    """An array touched only by top-level Tasklets (no kernel) should
    NOT be mirrored -- it stays CPU-only."""
    top = SDFG("host_only")
    top.add_array("host_only_arr", [1], dace.float64, transient=False,
                  storage=dtypes.StorageType.CPU_Heap)
    st = top.add_state()
    ac = st.add_access("host_only_arr")
    t = st.add_tasklet("w", {}, {"o"}, "o = 3.14")
    st.add_edge(t, "o", ac, None, mm.Memlet(data="host_only_arr", subset="0"))

    OffloadVelocityToGPU(top)
    assert "gpu_host_only_arr" not in top.arrays
    assert top.arrays["host_only_arr"].storage == dtypes.StorageType.CPU_Heap


def test_kernel_side_accessnode_retargeted_hostside_not():
    """Mixed: the same state can have a top-level Tasklet (host) and a
    top-level MapEntry (kernel). The AccessNode feeding the kernel is
    retargeted to ``gpu_``; the one bound to the tasklet stays."""
    top = SDFG("mixed")
    top.add_array("kern_arr", [4, 4], dace.float64, transient=False)
    top.add_array("host_arr", [1], dace.float64, transient=False)

    # Host tasklet: writes host_arr.
    host_st = top.add_state("host_state", is_start_block=True)
    host_ac = host_st.add_access("host_arr")
    host_t = host_st.add_tasklet("w", {}, {"o"}, "o = 1.0")
    host_st.add_edge(host_t, "o", host_ac, None,
                     mm.Memlet(data="host_arr", subset="0"))

    # Kernel state: reads kern_arr into a trivial map + dummy NSDFG.
    kern_st = top.add_state_after(host_st, label="kern_state")
    me, mx = kern_st.add_map("k", {"i": "0:4", "j": "0:4"})
    me.add_in_connector("IN_kern_arr")
    me.add_out_connector("OUT_kern_arr")
    mx.add_in_connector("IN_kern_arr")
    mx.add_out_connector("OUT_kern_arr")
    kern_read = kern_st.add_read("kern_arr")
    kern_write = kern_st.add_write("kern_arr")
    t = kern_st.add_tasklet("id", {"a"}, {"o"}, "o = a")
    kern_st.add_edge(kern_read, None, me, "IN_kern_arr",
                     mm.Memlet.from_array("kern_arr", top.arrays["kern_arr"]))
    kern_st.add_edge(me, "OUT_kern_arr", t, "a", mm.Memlet(data="kern_arr", subset="i, j"))
    kern_st.add_edge(t, "o", mx, "IN_kern_arr", mm.Memlet(data="kern_arr", subset="i, j"))
    kern_st.add_edge(mx, "OUT_kern_arr", kern_write, None,
                     mm.Memlet.from_array("kern_arr", top.arrays["kern_arr"]))

    OffloadVelocityToGPU(top)

    # kern_arr was mirrored, host_arr was not.
    assert "gpu_kern_arr" in top.arrays
    assert "gpu_host_arr" not in top.arrays

    # The top-level AccessNode inside host_state for host_arr keeps its
    # original name (host-side).
    for n in host_st.nodes():
        if isinstance(n, nodes.AccessNode):
            assert n.data == "host_arr"

    # The top-level AccessNodes inside kern_state for kern_arr got
    # retargeted to gpu_kern_arr.
    kern_access = [n for n in kern_st.nodes() if isinstance(n, nodes.AccessNode)]
    assert kern_access, "expected AccessNodes in the kernel state"
    for n in kern_access:
        assert n.data == "gpu_kern_arr", (
            f"kernel-side AccessNode should be retargeted, got data={n.data!r}")


def test_exclude_from_offload_keeps_array_cpu_side():
    """A kernel-side non-transient array listed in
    ``exclude_from_offload`` must NOT get a ``gpu_<name>`` mirror and
    must stay in the signature untouched -- this is the
    ``__CG_p_diag__m_max_vcfl_dyn`` round-trip case where the Fortran
    caller reads the value on the host after the SDFG returns."""
    top = _stage3_shaped_sdfg()
    sig_before = sorted(
        (n, type(d).__name__, tuple(str(x) for x in d.shape))
        for n, d in top.arrays.items() if not d.transient)

    OffloadVelocityToGPU(top, exclude_from_offload=("A",))

    # No gpu_ mirror for A -- it stayed CPU.
    assert "gpu_A" not in top.arrays
    # B did get mirrored.
    assert "gpu_B" in top.arrays
    # A's top-level descriptor unchanged: non-transient, same storage class.
    assert top.arrays["A"].transient is False
    assert top.arrays["A"].storage in (dtypes.StorageType.Default,
                                        dtypes.StorageType.CPU_Heap,
                                        dtypes.StorageType.Register)

    # Signature (non-transient entries) is still stable w.r.t. A's type/shape.
    sig_after = sorted(
        (n, type(d).__name__, tuple(str(x) for x in d.shape))
        for n, d in top.arrays.items() if not d.transient)
    for n_before in sig_before:
        name = n_before[0]
        matches = [n for n in sig_after if n[0] == name]
        assert matches, f"non-transient {name!r} disappeared from signature"
        assert matches[0] == n_before, (
            f"signature drift for {name!r}: before={n_before} after={matches[0]}")


def test_multi_sdfg_fails_on_multiple_end_states():
    """``OffloadVelocityToGPU`` assumes a single end state (where
    copy-out goes). Fail loudly if that invariant doesn't hold."""
    top = SDFG("multi_end")
    top.add_array("A", [1, 4], dace.float64, transient=False)
    top.add_array("B", [1, 4], dace.float64, transient=False)

    # Build a diamond: start -> mid1 + mid2 -> (two distinct terminals)
    s = top.add_state("start", is_start_block=True)
    end1 = top.add_state("end1")
    end2 = top.add_state("end2")
    top.add_edge(s, end1, dace.InterstateEdge(condition="1 == 1"))
    top.add_edge(s, end2, dace.InterstateEdge(condition="1 == 2"))
    # Force a kernel so mirror runs.
    me, mx = s.add_map("k", {"i": "0:4"})
    me.add_in_connector("IN_A")
    me.add_out_connector("OUT_A")
    mx.add_in_connector("IN_B")
    mx.add_out_connector("OUT_B")
    ar, bw = s.add_read("A"), s.add_write("B")
    t = s.add_tasklet("id", {"a"}, {"o"}, "o = a")
    s.add_edge(ar, None, me, "IN_A", mm.Memlet.from_array("A", top.arrays["A"]))
    s.add_edge(me, "OUT_A", t, "a", mm.Memlet(data="A", subset="0, i"))
    s.add_edge(t, "o", mx, "IN_B", mm.Memlet(data="B", subset="0, i"))
    s.add_edge(mx, "OUT_B", bw, None, mm.Memlet.from_array("B", top.arrays["B"]))

    with pytest.raises(AssertionError, match="one end state"):
        OffloadVelocityToGPU(top)


if __name__ == "__main__":
    test_schedules_assigned_gpu_at_top_sequential_inside()
    test_nontransient_arrays_get_gpu_mirror_and_copy_states()
    test_transient_arrays_promoted_to_gpu_global()
    test_host_only_nontransient_is_left_on_cpu()
    test_kernel_side_accessnode_retargeted_hostside_not()
    test_multi_sdfg_fails_on_multiple_end_states()
    print("all OffloadVelocityToGPU tests passed")
