"""Bridge E6/E7 layout configs into the legacy ``PERMUTE_CONFIGS`` dict shape.

Legacy stage drivers (``run_stage4_permutations.py``,
``run_stage8_permutations.py``, ``run_permutations.py``) consume::

    PERMUTE_CONFIGS = {
        "name": {
            "permute_map":         {"arr": [perm], ...},
            "inverse_permute_map": {"arr": [inv_perm], ...},
        },
        ...
    }

E7 emits ``PermuteConfig`` instances from its access-analysis JSON. This
module imports E7's loader (and curated set) and converts each
``PermuteConfig`` into the legacy dict shape so the legacy runners can
sweep configs that were originally specified for E6/E7.

The default candidates path points at E6's
``access_analysis/layout_candidates.json`` -- the same default E7 uses in
``utils/stages/stage6.py``.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Optional


# Resolve once. ``Path(__file__) -> .../E8_LegacyVT/utils/passes/load_e6_e7_configs.py``
# so parents[3] is ``.../Experiments``.
_E8_ROOT = Path(__file__).resolve().parents[2]
_EXPERIMENTS_ROOT = Path(__file__).resolve().parents[3]
_E7_ROOT = _EXPERIMENTS_ROOT / "E7_FullVelocityTendencies"
_E6_ROOT = _EXPERIMENTS_ROOT / "E6_VelocityTendencies"

DEFAULT_CANDIDATES_JSON: Path = _E6_ROOT / "access_analysis" / "layout_candidates.json"


def _import_e7_permute_configs():
    """Inject E7's package root into sys.path and import its config loader.

    Falls back to ``None`` if E7 is missing or its DaCe branch lacks the
    ``permute_layout`` module -- callers handle the empty-config case.
    """
    if not _E7_ROOT.is_dir():
        return None, None
    e7_str = str(_E7_ROOT)
    if e7_str not in sys.path:
        sys.path.insert(0, e7_str)
    try:
        from utils.passes.permute_configs import (  # type: ignore
            extended_configs_from_candidates,
        )
        from utils.passes.curated_configs import curated_configs  # type: ignore
    except Exception as exc:  # pragma: no cover - environment-dependent
        print(f"[load_e6_e7_configs] E7 import failed: {exc!r}", file=sys.stderr)
        return None, None
    return extended_configs_from_candidates, curated_configs


def _inverse(p: List[int]) -> List[int]:
    out = [0] * len(p)
    for i, v in enumerate(p):
        out[v] = i
    return out


def _to_legacy_entry(permute_map: Dict[str, List[int]],
                     inverse_permute_map: Optional[Dict[str, List[int]]] = None,
                     ) -> Dict[str, Dict[str, List[int]]]:
    pm = {k: list(v) for k, v in permute_map.items()}
    inv = (
        {k: list(v) for k, v in inverse_permute_map.items()}
        if inverse_permute_map
        else {k: _inverse(v) for k, v in pm.items()}
    )
    return {"permute_map": pm, "inverse_permute_map": inv}


def load_legacy_configs(
    candidates_json: Optional[Path] = None,
    sdfg_arrays: Optional[Dict[str, int]] = None,
) -> Dict[str, Dict[str, Dict[str, List[int]]]]:
    """Return E6/E7 configs in legacy ``PERMUTE_CONFIGS`` dict shape.

    Includes every name that E7's
    :func:`extended_configs_from_candidates` would emit -- the heuristic
    triple (``unpermuted``, ``nlev_first``, ``index_only``), per-nest
    ``nest_<nid>_<shape>`` entries, and the curated 95-cell sweep
    (``cv*_ch*_f*_s*_n*`` + ``curated_nlev_first`` / ``curated_index_only``).
    """
    extended, _curated = _import_e7_permute_configs()
    if extended is None:
        return {}

    json_path = Path(candidates_json) if candidates_json else DEFAULT_CANDIDATES_JSON
    if not json_path.is_file():
        print(f"[load_e6_e7_configs] candidates JSON not found: {json_path}",
              file=sys.stderr)
        return {}

    out: Dict[str, Dict[str, Dict[str, List[int]]]] = {}
    for cfg in extended(json_path, sdfg_arrays=sdfg_arrays):
        out[cfg.name] = _to_legacy_entry(cfg.permute_map, cfg.inverse_permute_map)
    return out


def merged_with_legacy(
    legacy_configs: Dict[str, Dict[str, Dict[str, List[int]]]],
    candidates_json: Optional[Path] = None,
    sdfg_arrays: Optional[Dict[str, int]] = None,
) -> Dict[str, Dict[str, Dict[str, List[int]]]]:
    """Return ``legacy_configs`` with E6/E7-derived entries layered in.

    Legacy entries win on name collision so that re-running an
    icon-artifacts config name reproduces icon-artifacts numerics
    bit-for-bit. New names contributed by E6/E7 (`unpermuted`,
    ``nlev_first``, ``index_only``, ``nest_*``, ``curated_*``) are added
    on top.
    """
    e6_e7 = load_legacy_configs(candidates_json=candidates_json, sdfg_arrays=sdfg_arrays)
    merged = dict(e6_e7)
    merged.update(legacy_configs)
    return merged


__all__ = [
    "DEFAULT_CANDIDATES_JSON",
    "load_legacy_configs",
    "merged_with_legacy",
]
