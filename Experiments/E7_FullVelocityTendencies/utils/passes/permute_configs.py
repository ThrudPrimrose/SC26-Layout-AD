"""Translate ``layout_candidates.json`` into a list of ``PermuteConfig``.

The JSON is produced by the SC26-Layout-AD access analysis (see
``Experiments/E6_VelocityTendencies/access_analysis/``). It clusters the
loop nests of velocity_advection by access pattern and lists the arrays
each nest touches. We emit:

* ``unpermuted`` — identity, no arrays permuted.
* ``nlev_first`` — every array reachable from a nest with ``shape='v.h'``
  is permuted to put the level (vertical) axis first.
* ``index_only`` — only connectivity / index arrays are permuted.

The 95-cell sweep from icon-artifacts is intentionally not ported; it is
data, and what the translator emits is one representative config per
single-axis decision (the user can hand-extend by listing more configs in
this file). Everything else is orchestration.
"""

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List

from .permute_layout import PermuteConfig

# Heuristic: 3-D arrays that fit (nproma, nlev, nblks) are level-firsted as
# [1, 0, 2]. 2-D (nproma, nblks) become [1, 0]. Connectivity index arrays
# (3-D triple of (nproma, nblks, nshape)) get an "n" permutation. We emit
# only one canonical permutation per group; extend by editing the maps
# below.
_THREE_D_LEVEL_FIRST: List[int] = [1, 0, 2]
_TWO_D_LEVEL_FIRST:   List[int] = [1, 0]
_CONN_PERM:           List[int] = [2, 0, 1]   # (nproma, nblks, nshape) -> (nshape, nproma, nblks)


# Forced overrides: arrays that should ALWAYS be permuted, regardless of
# which configuration is selected. The Fortran source declares
# ``levmask(nblks_c, nlev)`` (column-major: ``jb`` fast-varying), but
# f2dace lowers it as C row-major ``levmask[jb, jk]`` -- the OPPOSITE of
# the original memory order. The line-600 ``ANY(levmask(:, jk))`` reduce
# wants ``jb`` contiguous; transposing to ``[1, 0]`` (i.e.
# ``levmask[jk, jb]``) restores the Fortran-canonical layout AND fixes
# the reduce stride. Empirically validated to be uniformly better.
#
# Anything added here is merged into every emitted PermuteConfig
# (including ``unpermuted``), so the "baseline" config is more accurately
# "everything default EXCEPT these forced overrides".
# Both the host-side ``levmask`` and the GPU-mirror ``gpu_levmask`` are
# listed: stage 4's ``OffloadVelocityToGPU`` mirrors host arrays into
# ``gpu_<name>`` siblings, and post-stage-4 SDFGs reference the GPU
# name only. The bare ``levmask`` entry stays for callers that run
# permute_layout pre-stage-4 (e.g. test fixtures).
# ``levelmask`` and ``gpu_levelmask`` are 1-D ``(nlev,)``; the dim
# filter in ``_apply_force_permuted`` skips them automatically (a
# 1-element permutation has no effect anyway).
#
# Empty by design: ``levmask`` is grouped on the new ``lv`` axis of the
# curated / named cross-product, so it is permuted only when the
# selected config asks for it. Always-on force-permute is wrong because
# ``unpermuted`` would no longer be a true identity baseline.
_FORCE_PERMUTED: Dict[str, List[int]] = {
}

# Per-array map for the ``lv`` axis: when the cross-product cell sets
# ``lv=1``, this map is merged into the config's ``permute_map``. The
# post-stage-4 SDFG only carries the ``gpu_levmask`` GPU mirror (the
# host ``levmask`` is renamed in place by ``OffloadVelocityToGPU``).
_LV_PERMUTED: Dict[str, List[int]] = {
    "gpu_levmask": [1, 0],
}


def _apply_force_permuted(configs: List['PermuteConfig'],
                          sdfg_arrays: Dict[str, int] | None) -> List['PermuteConfig']:
    """Merge ``_FORCE_PERMUTED`` into every config's ``permute_map``.

    Returns a NEW list of ``PermuteConfig`` instances so the auto-computed
    ``inverse_permute_map`` is rebuilt for each one. Force overrides are
    skipped silently when the array is absent / has a different
    dimensionality in ``sdfg_arrays`` -- the same dim-filter logic used
    for the heuristic configs.
    """
    forced: Dict[str, List[int]] = {}
    for name, perm in _FORCE_PERMUTED.items():
        if sdfg_arrays is not None:
            ndim = sdfg_arrays.get(name)
            if ndim is None or ndim != len(perm):
                continue
        forced[name] = list(perm)
    if not forced:
        return configs
    out: List[PermuteConfig] = []
    for c in configs:
        merged = dict(c.permute_map)
        for name, perm in forced.items():
            # Force overrides any heuristic permutation -- the heuristic
            # has no way to disambiguate (e.g.) a 2D ``levmask`` from the
            # 3D arrays it shares a v.h nest with, and would otherwise
            # silently emit an N-D permutation that doesn't match the
            # array's real rank.
            merged[name] = list(perm)
        out.append(PermuteConfig(name=c.name, permute_map=merged))
    return out


def fortran_to_sdfg_array_name(s: str) -> str:
    """Map a Fortran-flavored array reference to the SDFG-internal name.

    >>> fortran_to_sdfg_array_name('p_diag%vn_ie')
    '__CG_p_diag__m_vn_ie'
    >>> fortran_to_sdfg_array_name('z_kin_hor_e')
    'z_kin_hor_e'
    """
    if '%' not in s:
        return s
    return '__CG_' + s.replace('%', '__m_').strip()


def _resolve_gpu_name(host_name: str,
                      sdfg_arrays: Dict[str, int] | None) -> str | None:
    """Pick the post-stage-4 name for ``host_name``.

    Stage 4's ``OffloadVelocityToGPU`` either renames a pure-scratch
    transient in place to ``gpu_<name>`` (host gone) or mirrors a
    non-transient host array into a sibling ``gpu_<name>`` (both
    present). After stage 4 the kernel side always references the
    ``gpu_`` form, so the permute map needs to be keyed on it.

    Returns the SDFG-resident name, or ``None`` when neither variant
    exists. When ``sdfg_arrays`` is ``None`` (callers that just want
    name normalization without filtering), prefer the ``gpu_`` form
    by default since this module is post-stage-4 only.
    """
    gpu_name = "gpu_" + host_name
    if sdfg_arrays is None:
        return gpu_name
    if gpu_name in sdfg_arrays:
        return gpu_name
    if host_name in sdfg_arrays:
        return host_name
    return None


def _array_dim(name: str, sdfg_arrays: Dict[str, int] | None) -> int | None:
    if sdfg_arrays is None:
        return None
    return sdfg_arrays.get(name)


def configs_from_candidates(json_path: str | Path,
                            sdfg_arrays: Dict[str, int] | None = None) -> List[PermuteConfig]:
    """Return a list of ``PermuteConfig`` derived from ``layout_candidates.json``.

    ``sdfg_arrays`` is an optional ``{name: ndim}`` map; when provided we
    skip arrays whose dimensionality doesn't match the permutation we
    want to apply, instead of failing inside ``PermuteDimensions``.
    """
    data = json.loads(Path(json_path).read_text())

    nlev_first_3d: set[str] = set()
    nlev_first_2d: set[str] = set()
    conn_arrays:    set[str] = set()

    for nest in data.get('all_nests', []):
        shape = nest.get('shape', '')
        for fortran_name in nest.get('arrays', []):
            host = fortran_to_sdfg_array_name(fortran_name)
            name = _resolve_gpu_name(host, sdfg_arrays)
            if name is None:
                continue
            if shape == 'v.h':
                nlev_first_3d.add(name)
            elif shape == 'h':
                nlev_first_2d.add(name)
            elif shape == 'v':
                # vertical-only arrays: leaving identity is the safe default
                continue
        # Connectivity indices live in the data even without an explicit
        # 'shape' label; the analysis tags them via class_label.
        if 'connectivity' in (nest.get('class_label') or '').lower():
            for fortran_name in nest.get('arrays', []):
                host = fortran_to_sdfg_array_name(fortran_name)
                name = _resolve_gpu_name(host, sdfg_arrays)
                if name is not None:
                    conn_arrays.add(name)

    def _filter_by_dim(names: set[str], wanted_dim: int) -> set[str]:
        if sdfg_arrays is None:
            return names
        return {n for n in names if sdfg_arrays.get(n) == wanted_dim}

    # Build the base configs. ``nlev_first`` and ``index_only`` use the
    # researcher-tuned ``_CURATED_NLEV_FIRST`` / ``_CURATED_INDEX_ONLY``
    # maps so coverage is complete: the JSON heuristic alone missed
    # connectivity arrays (their nests aren't tagged ``v.h``), 4-D
    # ``ddt_*_pc`` fields (the heuristic only handled rank 2 and 3),
    # and a few 3-D arrays that the access analysis didn't reach. The
    # JSON-derived sets are still computed for the per-nest configs
    # below and as a sanity reference.
    base: List[PermuteConfig] = [PermuteConfig(name='unpermuted', permute_map={})]

    # Late import to avoid a circular dep at module load: curated_configs
    # imports from this module via ``permute_layout``.
    from .curated_configs import _CURATED_NLEV_FIRST, _CURATED_INDEX_ONLY

    nlev_first_map = {n: list(p) for n, p in _CURATED_NLEV_FIRST.items()}
    if sdfg_arrays is not None:
        nlev_first_map = {n: p for n, p in nlev_first_map.items()
                          if sdfg_arrays.get(n) == len(p)}
    if nlev_first_map:
        base.append(PermuteConfig(name='nlev_first', permute_map=nlev_first_map))

    index_only_map = {n: list(p) for n, p in _CURATED_INDEX_ONLY.items()}
    if sdfg_arrays is not None:
        index_only_map = {n: p for n, p in index_only_map.items()
                          if sdfg_arrays.get(n) == len(p)}
    if index_only_map:
        base.append(PermuteConfig(name='index_only', permute_map=index_only_map))

    expanded = _double_on_sm(_double_on_lv(base, sdfg_arrays))
    return _apply_force_permuted(expanded, sdfg_arrays)


def _double_on_lv(configs: List['PermuteConfig'],
                  sdfg_arrays: Dict[str, int] | None) -> List['PermuteConfig']:
    """Expand each input config into a ``_lv0`` (identity for the
    levmask group) and ``_lv1`` (levmask permuted) pair. Filtering by
    SDFG-side dimensionality is applied to the ``lv=1`` overlay --
    arrays that don't exist or have a different rank are silently
    dropped."""
    lv1_overlay: Dict[str, List[int]] = {}
    for name, perm in _LV_PERMUTED.items():
        if sdfg_arrays is not None:
            ndim = sdfg_arrays.get(name)
            if ndim is None or ndim != len(perm):
                continue
        lv1_overlay[name] = list(perm)

    out: List[PermuteConfig] = []
    for c in configs:
        out.append(PermuteConfig(name=f"{c.name}_lv0",
                                 permute_map=dict(c.permute_map),
                                 shuffle_map=c.shuffle_map))
        merged = dict(c.permute_map)
        merged.update(lv1_overlay)
        out.append(PermuteConfig(name=f"{c.name}_lv1",
                                 permute_map=merged,
                                 shuffle_map=c.shuffle_map))
    return out


def _double_on_sm(configs: List['PermuteConfig']) -> List['PermuteConfig']:
    """Expand each input config into a ``_sm0`` (loops left alone) and
    ``_sm1`` (loop axes shuffled to match the array permutation) pair.
    The two variants share the same ``permute_map`` and differ only in
    ``shuffle_map`` -- so the SDFG codegen / launch grid changes but
    the on-disk array layout is identical."""
    out: List[PermuteConfig] = []
    for c in configs:
        out.append(PermuteConfig(name=f"{c.name}_sm0",
                                 permute_map=dict(c.permute_map),
                                 shuffle_map=False))
        out.append(PermuteConfig(name=f"{c.name}_sm1",
                                 permute_map=dict(c.permute_map),
                                 shuffle_map=True))
    return out


def _nest_config_name(nid: int, shape: str) -> str:
    return f"nest_{nid}_{shape.replace('.', '')}"


def _per_nest_permute_map(nest: dict,
                          sdfg_arrays: Dict[str, int] | None) -> Dict[str, List[int]]:
    shape = nest.get('shape', '')
    pmap: Dict[str, List[int]] = {}
    for fortran_name in nest.get('arrays', []):
        host = fortran_to_sdfg_array_name(fortran_name)
        name = _resolve_gpu_name(host, sdfg_arrays)
        if name is None:
            continue
        ndim = sdfg_arrays.get(name) if sdfg_arrays is not None else None
        if shape == 'v.h':
            if ndim is None or ndim == 3:
                pmap[name] = list(_THREE_D_LEVEL_FIRST)
            elif ndim == 2:
                pmap[name] = list(_TWO_D_LEVEL_FIRST)
        elif shape == 'h':
            if ndim is None or ndim == 2:
                pmap[name] = list(_TWO_D_LEVEL_FIRST)
    if 'connectivity' in (nest.get('class_label') or '').lower():
        for fortran_name in nest.get('arrays', []):
            host = fortran_to_sdfg_array_name(fortran_name)
            name = _resolve_gpu_name(host, sdfg_arrays)
            if name is None:
                continue
            if sdfg_arrays is not None and sdfg_arrays.get(name) != 3:
                continue
            pmap[name] = list(_CONN_PERM)
    return pmap


def extended_configs_from_candidates(json_path: str | Path,
                                     sdfg_arrays: Dict[str, int] | None = None) -> List[PermuteConfig]:
    """``configs_from_candidates`` plus per-nest + curated configs.

    Yields, in order:

    1. The named heuristic configs (``unpermuted``, ``nlev_first``,
       ``index_only``) emitted by :func:`configs_from_candidates`.
    2. One ``nest_<nid>_<shape>`` per ``all_nests`` entry whose ``shape``
       is ``h`` or ``v.h``. ``v``-only nests would map to identity and
       are skipped to avoid duplicating ``unpermuted``.
    3. The researcher-tuned curated set from
       :func:`utils.passes.curated_configs.curated_configs` -- 94
       sweep cells (``cv<0|1>_ch<0|1>_f<0|1>_s<0|1>_n<perm>``) plus
       ``curated_nlev_first`` and ``curated_index_only``.

    Names already produced by an earlier stage are skipped, so the
    curated 95-cell sweep never collides with the heuristic outputs.
    """
    from .curated_configs import curated_configs

    configs = list(configs_from_candidates(json_path, sdfg_arrays=sdfg_arrays))
    seen = {c.name for c in configs}

    data = json.loads(Path(json_path).read_text())
    for nest in data.get('all_nests', []):
        shape = nest.get('shape', '')
        if shape not in ('h', 'v.h'):
            continue
        nid = nest.get('nid')
        if nid is None:
            continue
        name = _nest_config_name(int(nid), shape)
        if name in seen:
            continue
        pmap = _per_nest_permute_map(nest, sdfg_arrays)
        if not pmap:
            continue
        configs.append(PermuteConfig(name=name, permute_map=pmap))
        seen.add(name)

    for cfg in curated_configs(sdfg_arrays=sdfg_arrays):
        if cfg.name in seen:
            continue
        configs.append(cfg)
        seen.add(cfg.name)

    # E6 V_k aliases -- explicit named configs that map to the V1 / V2 /
    # V6 empirical-winner variants from
    # ``E6_VelocityTendencies/generate_v123_candidates.py``. These are
    # what the AD reproduces from the paper (§IV-D, "indirect stencils
    # favor vertical-first" + Table IV showing V6 as the v_first/AoS
    # winner). Defined as aliases over already-emitted configs so the
    # AD reader can submit ``winner_v1`` / ``winner_v2`` / ``winner_v6``
    # without translating axis-flag names back to V_k.
    #
    #   winner_v1 -- h_first + SoA-conn (identity baseline)
    #   winner_v2 -- h_first + AoS-conn (only connectivity permuted to
    #                the validated [0, 2, 1])
    #   winner_v6 -- v_first + AoS-conn (full level-first across all
    #                E6-classified groups + connectivity at [0, 2, 1])
    _add_winner_alias(configs, seen, 'winner_v1', 'unpermuted_lv0_sm0')
    _add_winner_alias(configs, seen, 'winner_v2', 'index_only_lv0_sm0')
    _add_winner_alias(configs, seen, 'winner_v6', 'nlev_first_lv1_sm0')

    # Apply force-permute overrides (e.g. levmask -> [1, 0]) uniformly
    # across every emitted config -- including ``unpermuted`` and the
    # listed nest_<...>/cohort_<...> configs that didn't come through
    # ``configs_from_candidates`` / ``curated_configs``.
    return _apply_force_permuted(configs, sdfg_arrays)


def _add_winner_alias(configs: List[PermuteConfig], seen: set,
                      alias: str, source: str) -> None:
    """Append ``alias`` as a copy of the config named ``source``. Silent
    no-op if ``source`` isn't in ``configs`` (e.g. SDFG-side filtering
    dropped its permute map)."""
    if alias in seen:
        return
    src = next((c for c in configs if c.name == source), None)
    if src is None:
        return
    configs.append(PermuteConfig(
        name=alias,
        permute_map=dict(src.permute_map),
        shuffle_map=src.shuffle_map,
    ))
    seen.add(alias)
