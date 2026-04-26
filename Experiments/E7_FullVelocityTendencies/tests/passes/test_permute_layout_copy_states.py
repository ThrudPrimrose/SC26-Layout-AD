"""Tests for ``permute_layout`` ordering relative to GPU copy states.

After stage 4, the SDFG carries explicit copy-in / copy-out states::

    _cpu_to_gpu_copy_in  (X     -> gpu_X)
    _sync_after_copy_in  (cuStreamSync)
    <body states using gpu_*>
    _gpu_to_cpu_copy_out (gpu_X -> X)
    _sync_after_copy_out (cuStreamSync)

The cudaMemcpy in copy_in / copy_out is a flat byte blit, so it must
target the *unpermuted* ``gpu_X``. The actual layout transpose is a
GPU<->GPU Map+Tasklet kernel inserted as a fresh state
(``permute_after_<gpu_X>`` right after copy_in;
``permute_before_<gpu_X>`` right before copy_out). The
``_restore_boundary_copy_targets`` post-pass in
:mod:`utils.passes.permute_layout` enforces this -- without it,
``PermuteDimensions``'s body-rename loop redirects the cudaMemcpy
output to ``permuted_gpu_X`` and bytes land in the wrong layout.

These tests are skipped when DaCe doesn't ship the ``permute_dimensions``
module (i.e. on branches where the layout pass is being rewritten).
"""

import pytest

import dace
from dace import dtypes, memlet as mm
from dace.sdfg import SDFG

pytest.importorskip("dace.transformation.layout.permute_dimensions")

from utils.passes.permute_layout import PermuteConfig, permute_layout  # noqa: E402


def _stage4_shaped_sdfg(M: int = 4, N: int = 4, K: int = 3) -> SDFG:
    """Synthetic stage-4-shaped SDFG.

    Layout:
        non-transient X, Y  (host-side)
        transient gpu_X, gpu_Y  (GPU_Global storage)

    States (linear chain):
        _cpu_to_gpu_copy_in -> _sync_after_copy_in
        -> body  (kernel reads gpu_X, writes gpu_Y)
        -> _gpu_to_cpu_copy_out -> _sync_after_copy_out

    The body has a top-level GPU_Device map; gpu_X / gpu_Y are
    GPU_Global. Modeled on what stage 4 actually emits.
    """
    sdfg = SDFG("stage4_synth")
    sdfg.add_array("X", [M, N, K], dace.float64, transient=False)
    sdfg.add_array("Y", [M, N, K], dace.float64, transient=False)
    sdfg.add_array("gpu_X", [M, N, K], dace.float64,
                   transient=True,
                   storage=dtypes.StorageType.GPU_Global,
                   lifetime=dtypes.AllocationLifetime.Persistent)
    sdfg.add_array("gpu_Y", [M, N, K], dace.float64,
                   transient=True,
                   storage=dtypes.StorageType.GPU_Global,
                   lifetime=dtypes.AllocationLifetime.Persistent)

    # _cpu_to_gpu_copy_in
    s_in = sdfg.add_state("_cpu_to_gpu_copy_in", is_start_block=True)
    rx = s_in.add_read("X")
    wgx = s_in.add_write("gpu_X")
    s_in.add_edge(rx, None, wgx, None, mm.Memlet.from_array("X", sdfg.arrays["X"]))

    # _sync_after_copy_in
    s_sync_in = sdfg.add_state_after(s_in, "_sync_after_copy_in")

    # body
    s_body = sdfg.add_state_after(s_sync_in, "body")
    rgx = s_body.add_read("gpu_X")
    wgy = s_body.add_write("gpu_Y")
    me, mx = s_body.add_map(
        "kernel",
        {"i": f"0:{M}", "j": f"0:{N}", "k": f"0:{K}"},
        schedule=dtypes.ScheduleType.GPU_Device,
    )
    me.add_in_connector("IN_X")
    me.add_out_connector("OUT_X")
    mx.add_in_connector("IN_Y")
    mx.add_out_connector("OUT_Y")
    t = s_body.add_tasklet("scale", {"a"}, {"o"}, "o = 2.0 * a")
    s_body.add_edge(rgx, None, me, "IN_X",
                    mm.Memlet.from_array("gpu_X", sdfg.arrays["gpu_X"]))
    s_body.add_edge(me, "OUT_X", t, "a", mm.Memlet(data="gpu_X", subset="i, j, k"))
    s_body.add_edge(t, "o", mx, "IN_Y", mm.Memlet(data="gpu_Y", subset="i, j, k"))
    s_body.add_edge(mx, "OUT_Y", wgy, None,
                    mm.Memlet.from_array("gpu_Y", sdfg.arrays["gpu_Y"]))

    # _gpu_to_cpu_copy_out
    s_out = sdfg.add_state_after(s_body, "_gpu_to_cpu_copy_out")
    rgy = s_out.add_read("gpu_Y")
    wy = s_out.add_write("Y")
    s_out.add_edge(rgy, None, wy, None, mm.Memlet.from_array("gpu_Y", sdfg.arrays["gpu_Y"]))

    # _sync_after_copy_out
    sdfg.add_state_after(s_out, "_sync_after_copy_out")

    return sdfg


def _state_by_label(sdfg: SDFG, label: str):
    return next(st for st in sdfg.all_states() if st.label == label)


def _memlets_targeting(state, name: str):
    """Memlets in ``state`` that read from or write into ``name``."""
    return [e.data for e in state.edges() if e.data is not None and e.data.data == name]


def _access_node_names(state) -> set:
    """Set of AccessNode.data values currently present in ``state``."""
    import dace as _dace
    return {n.data for n in state.nodes()
            if isinstance(n, _dace.nodes.AccessNode) and n.data is not None}


def test_copy_in_keeps_unpermuted_gpu_target():
    """``_cpu_to_gpu_copy_in`` must still hold an AccessNode for
    ``gpu_X`` (not ``permuted_gpu_X``). The cudaMemcpy is a flat byte
    blit; pointing it at ``permuted_gpu_X`` would land bytes in a
    permuted-stride array. ``_restore_boundary_copy_targets`` undoes
    the rename PermuteDimensions's body-rename loop applied here."""
    sdfg = _stage4_shaped_sdfg()
    cfg = PermuteConfig(name="t", permute_map={"gpu_X": [1, 0, 2], "gpu_Y": [1, 0, 2]})
    permute_layout(sdfg, cfg, shuffle_map=False)
    sdfg.validate()

    copy_in = _state_by_label(sdfg, "_cpu_to_gpu_copy_in")
    names = _access_node_names(copy_in)
    assert "gpu_X" in names, (
        f"copy_in lost its gpu_X AccessNode; boundary-strip post-pass "
        f"failed. Got: {sorted(names)}"
    )
    assert "permuted_gpu_X" not in names, (
        f"copy_in still has permuted_gpu_X AccessNode; boundary-strip "
        f"post-pass did not undo the rename. Got: {sorted(names)}"
    )


def test_copy_out_keeps_unpermuted_gpu_source():
    """Mirror: cudaMemcpy reads ``gpu_Y`` (unpermuted) and writes the
    host-side ``Y``. The ``permute_before_<gpu_Y>`` state inserted just
    before is what actually transposes ``permuted_gpu_Y`` -> ``gpu_Y``."""
    sdfg = _stage4_shaped_sdfg()
    cfg = PermuteConfig(name="t", permute_map={"gpu_Y": [1, 0, 2]})
    permute_layout(sdfg, cfg, shuffle_map=False)
    sdfg.validate()

    copy_out = _state_by_label(sdfg, "_gpu_to_cpu_copy_out")
    names = _access_node_names(copy_out)
    assert "gpu_Y" in names, (
        f"copy_out lost its gpu_Y AccessNode; boundary-strip post-pass "
        f"failed. Got: {sorted(names)}"
    )
    assert "permuted_gpu_Y" not in names, (
        f"copy_out still has permuted_gpu_Y AccessNode; boundary-strip "
        f"post-pass did not undo the rename. Got: {sorted(names)}"
    )
    assert "Y" in names, "copy_out must still write the host-side Y array"


def test_permute_after_inserted_for_input_transient():
    """``PermuteDimensions``'s per-transient logic inserts a
    ``permute_after_<gpu_X>`` state right after the copy-in containing
    a GPU<->GPU Map+Tasklet that does the actual transpose."""
    sdfg = _stage4_shaped_sdfg()
    cfg = PermuteConfig(name="t", permute_map={"gpu_X": [1, 0, 2]})
    permute_layout(sdfg, cfg, shuffle_map=False)
    sdfg.validate()

    labels = {st.label for st in sdfg.all_states()}
    assert "permute_after_gpu_X" in labels, (
        f"expected permute_after_gpu_X state to be inserted; got {sorted(labels)}"
    )


def test_permute_before_inserted_for_output_transient():
    """Mirror: ``permute_before_<gpu_Y>`` is inserted right before the
    copy-out, holding the inverse-transpose Map+Tasklet."""
    sdfg = _stage4_shaped_sdfg()
    cfg = PermuteConfig(name="t", permute_map={"gpu_Y": [1, 0, 2]})
    permute_layout(sdfg, cfg, shuffle_map=False)
    sdfg.validate()

    labels = {st.label for st in sdfg.all_states()}
    assert "permute_before_gpu_Y" in labels, (
        f"expected permute_before_gpu_Y state to be inserted; got {sorted(labels)}"
    )


def test_permute_layout_returns_zero_on_empty_map():
    """Sanity: an empty permute_map is a no-op and must not insert
    any state."""
    sdfg = _stage4_shaped_sdfg()
    n_states_before = sum(1 for _ in sdfg.all_states())
    cfg = PermuteConfig(name="noop", permute_map={})
    n = permute_layout(sdfg, cfg, shuffle_map=False)
    assert n == 0
    n_states_after = sum(1 for _ in sdfg.all_states())
    assert n_states_after == n_states_before
