"""Layout permutation driver for stage6.

Wraps :class:`dace.transformation.layout.PermuteDimensions` with a
``PermuteConfig`` dataclass so callers can serialize / iterate over a list
of named permutation choices instead of constructing the pass directly.

Map-dim shuffling (``MapDimShuffle``) is applied opportunistically when
available; pre-yakup/dev DaCe forks may lack it, in which case the wrapper
falls back to the bare permute.

Boundary-strip post-pass
------------------------
``PermuteDimensions``'s body-rename loop touches every state that is not
in its ``permute_states_to_skip`` set, including the stage-4 boundary
states (``_cpu_to_gpu_copy_in`` / ``_gpu_to_cpu_copy_out``). For every
``gpu_X`` in the permute map, the rename redirects the AccessNode and
the cudaMemcpy tasklet's outgoing edge from ``gpu_X`` to
``permuted_gpu_X`` -- but the cudaMemcpy is a flat byte blit; pointing
it at the permuted-stride array would land bytes in the wrong layout.
The pass already inserts ``permute_after_<gpu_X>`` /
``permute_before_<gpu_X>`` states that perform the actual GPU<->GPU
transpose, so the boundary copies must stay on the unpermuted name.
``_restore_boundary_copy_targets`` undoes the boundary rename after
the pass returns.
"""

from dataclasses import dataclass, field
from typing import Dict, List

import dace


def _inverse(p: List[int]) -> List[int]:
    out = [0] * len(p)
    for i, v in enumerate(p):
        out[v] = i
    return out


@dataclass
class PermuteConfig:
    name: str
    permute_map: Dict[str, List[int]]
    inverse_permute_map: Dict[str, List[int]] = field(default_factory=dict)
    # Whether ``permute_layout`` should also reorder the corresponding
    # map dimensions to match the array permutation. This is the
    # ``_sm{0,1}`` axis of the curated / named sweep -- ``_sm0`` keeps
    # the original loop nest, ``_sm1`` shuffles loop axes to keep
    # stride-1 access aligned with the new array layout.
    shuffle_map: bool = True

    def __post_init__(self):
        if not self.inverse_permute_map:
            self.inverse_permute_map = {k: _inverse(v) for k, v in self.permute_map.items()}


_COPY_IN_LABEL = "_cpu_to_gpu_copy_in"
_COPY_OUT_LABEL = "_gpu_to_cpu_copy_out"


def _restore_boundary_copy_targets(sdfg: dace.SDFG) -> None:
    """In ``_cpu_to_gpu_copy_in`` / ``_gpu_to_cpu_copy_out``, replace any
    ``permuted_<X>`` reference (AccessNode.data, memlet.data) with the
    unprefixed name and a full-extent memlet on the original shape.

    The cudaMemcpyAsync emitted by ``lift_full_copies_to_memcpy_tasklets``
    is a flat byte blit. After ``PermuteDimensions``, its outgoing edge
    has been redirected from ``gpu_X`` to ``permuted_gpu_X`` -- which
    would land original-layout bytes in a permuted-stride array. The
    per-transient ``permute_after_<gpu_X>`` / ``permute_before_<gpu_X>``
    states inserted by the pass do the actual GPU<->GPU transpose;
    keeping the cudaMemcpy on ``gpu_X`` is what we need.
    """
    for state in sdfg.all_states():
        if state.label not in (_COPY_IN_LABEL, _COPY_OUT_LABEL):
            continue
        for n in list(state.data_nodes()):
            if n.data and n.data.startswith("permuted_"):
                orig = n.data[len("permuted_"):]
                if orig in sdfg.arrays:
                    n.data = orig
        for e in list(state.edges()):
            if e.data is None or e.data.data is None:
                continue
            if not e.data.data.startswith("permuted_"):
                continue
            orig = e.data.data[len("permuted_"):]
            if orig in sdfg.arrays:
                e.data = dace.Memlet.from_array(orig, sdfg.arrays[orig])


def permute_layout(sdfg: dace.SDFG, config: PermuteConfig, shuffle_map: bool = True) -> int:
    """Apply ``config`` to ``sdfg`` in place. Returns the number of permuted arrays.

    Stage 4's ``_cpu_to_gpu_copy_in`` / ``_gpu_to_cpu_copy_out`` states
    carry full-extent CPU<->GPU copies as implicit ``AccessNode ->
    AccessNode`` edges. ``PermuteDimensions``'s ``require_core_dialect``
    gate refuses any SDFG with implicit AN->AN edges, so we run
    ``lift_full_copies_to_memcpy_tasklets`` first to convert each
    boundary copy into a ``memcpy_<src>_to_<dst>`` ``cudaMemcpyAsync``
    tasklet. The pass's per-transient logic then locates those tasklets
    via ``_find_init_copy_state`` / ``_find_final_copy_state`` and
    inserts ``permute_after_<gpu_X>`` right after the copy-in and
    ``permute_before_<gpu_X>`` right before the copy-out, each holding
    a GPU<->GPU Map+Tasklet transpose. After the pass returns, this
    wrapper restores the boundary AccessNodes / memlets to the
    unpermuted ``gpu_X`` so the cudaMemcpy stays on the original
    layout.
    """
    if not config.permute_map:
        return 0
    # Deferred imports: the listing tools (``list_layout_configs.py``)
    # only need ``PermuteConfig`` and shouldn't fail to load when DaCe
    # is on a branch that does not yet ship ``permute_dimensions``.
    from dace.transformation.layout.permute_dimensions import PermuteDimensions
    from .lift_full_copies_to_memcpy_tasklets import lift_full_copies_to_memcpy_tasklets

    # Lift implicit boundary CPU<->GPU copies to memcpy tasklets so the
    # core-dialect gate accepts the SDFG. Idempotent: a second invocation
    # on an already-lifted SDFG is a no-op (no implicit AN->AN edges
    # remain to match).
    lift_full_copies_to_memcpy_tasklets(sdfg)

    PermuteDimensions(
        permute_map=config.permute_map,
        add_permute_maps=True,
        column_major=False,
    ).apply_pass(sdfg=sdfg, pipeline_results={})

    _restore_boundary_copy_targets(sdfg)

    if shuffle_map:
        try:
            from dace.transformation.dataflow import MapDimShuffle
        except ImportError:
            MapDimShuffle = None
        if MapDimShuffle is not None:
            for state in sdfg.all_states():
                for node in list(state.nodes()):
                    if not isinstance(node, dace.nodes.MapEntry):
                        continue
                    if len(node.map.params) < 2:
                        continue
                    try:
                        MapDimShuffle.apply_to(sdfg=sdfg, map_entry=node)
                    except Exception:
                        # Best effort: not every map admits shuffling.
                        continue

    sdfg.validate()
    return len(config.permute_map)
