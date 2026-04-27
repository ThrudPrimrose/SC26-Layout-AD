"""Inspector: list every stage-5a ``--config`` name available against
the stage-4 SDFGs.

Reads ``layout_candidates.json`` (E6 access-analysis output) and prints
to stdout (or writes to ``--output``) a JSON document with two arrays:

    "named":  the 3 hardcoded names returned by ``configs_from_candidates``
              (``unpermuted``, ``nlev_first``, ``index_only``).
    "listed": one entry per ``all_nests[]`` in the JSON whose ``shape`` is
              ``h`` or ``v.h``. Names follow ``nest_<nid>_<shape>`` with
              ``v.h`` shortened to ``vh``.

Both kinds are runnable via
``python -m utils.stages.stage5a --config <name>`` (and the same
``--config`` is accepted by ``stage5b`` for the levmask + JSON
combined permutation).

This script is purely an inspector; it does not generate submission
scripts. Use the static ``run_{daint,beverin}.sh`` and override the
``CONFIGS`` env var to pick the configs you want to run.
"""

import argparse
import json
import sys
from pathlib import Path
from typing import List, Dict


_E6_DEFAULT = (
    Path(__file__).resolve().parent.parent.parent
    / "E6_VelocityTendencies"
    / "access_analysis"
    / "layout_candidates.json"
)


def _named_entries(json_path: Path) -> List[Dict]:
    """Run configs_from_candidates and report only the named configs it
    actually emits. ``index_only`` is suppressed when no connectivity
    arrays show up in the JSON, so listing it would falsely advertise a
    runnable config."""
    e7_root = Path(__file__).resolve().parent.parent
    if str(e7_root) not in sys.path:
        sys.path.insert(0, str(e7_root))
    from utils.passes.permute_configs import configs_from_candidates  # noqa: E402

    return [{"name": c.name, "kind": "named"} for c in configs_from_candidates(json_path)]


def _curated_entries() -> List[Dict]:
    """Surface the researcher-tuned configs as a third category."""
    e7_root = Path(__file__).resolve().parent.parent
    if str(e7_root) not in sys.path:
        sys.path.insert(0, str(e7_root))
    from utils.passes.curated_configs import curated_configs  # noqa: E402

    return [
        {"name": c.name, "kind": "curated", "n_arrays": len(c.permute_map)}
        for c in curated_configs()
    ]


def _listed_entries(data: Dict) -> List[Dict]:
    out: List[Dict] = []
    seen: set = set()
    for nest in data.get("all_nests", []) or []:
        shape = nest.get("shape", "")
        if shape not in ("h", "v.h"):
            continue
        nid = nest.get("nid")
        if nid is None:
            continue
        name = f"nest_{int(nid)}_{shape.replace('.', '')}"
        if name in seen:
            continue
        seen.add(name)
        out.append({
            "name": name,
            "kind": "listed",
            "nid": int(nid),
            "shape": shape,
            "n_arrays": int(nest.get("n_arrays", len(nest.get("arrays", []) or []))),
        })
    return out


def main(argv=None):
    argp = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    argp.add_argument(
        "--candidates",
        type=Path,
        default=_E6_DEFAULT,
        help="path to E6 layout_candidates.json (default: %(default)s).",
    )
    argp.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).resolve().parent / "submissions" / "layout_configs.json",
        help="path to write the JSON listing (default: %(default)s).",
    )
    argp.add_argument(
        "--print", dest="print_only", action="store_true",
        help="print human-readable listing to stdout instead of writing JSON.",
    )
    args = argp.parse_args(argv)

    if not args.candidates.is_file():
        raise SystemExit(f"[list_layout_configs] missing layout_candidates.json: {args.candidates}")

    data = json.loads(args.candidates.read_text())
    named = _named_entries(args.candidates)
    listed = _listed_entries(data)
    curated = _curated_entries()

    if args.print_only:
        print(f"[list_layout_configs] source: {args.candidates}")
        print(f"[list_layout_configs] {len(named)} named + {len(listed)} listed + "
              f"{len(curated)} curated configs")
        for kind, rows in (("named", named), ("listed", listed), ("curated", curated)):
            if not rows:
                continue
            print(f"\n  {kind} ({len(rows)}):")
            for r in rows:
                extra = ""
                if "n_arrays" in r:
                    extra = f"  ({r['n_arrays']} array perms)"
                elif "shape" in r:
                    extra = f"  shape={r['shape']}"
                print(f"    {r['name']}{extra}")
        return

    args.output.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "source": str(args.candidates),
        "named": named,
        "listed": listed,
        "curated": curated,
    }
    args.output.write_text(json.dumps(payload, indent=2) + "\n")

    print(f"[list_layout_configs] {len(named)} named + {len(listed)} listed + "
          f"{len(curated)} curated -> {args.output}")


if __name__ == "__main__":
    main()
