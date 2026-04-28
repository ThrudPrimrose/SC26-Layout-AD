#!/usr/bin/env python3
"""
Generate all valid layout permutation configs, compile, and run.

Five groups:
  cv  COMPUTE_VERT   binary [0,1,2] or [1,0,2]
  ch  COMPUTE_HORIZ  binary [0,1,2] or [1,0,2]
  f   FIELDS         binary [0,1,2] or [1,0,2]
  s   STENCIL        binary [0,1,2] or [1,0,2]
  n   CONN           all 6 permutations of [0,1,2]

Total: 2 x 2 x 2 x 2 x 6 - 1 = 95 configs.
Each config compiles to 2 executables: _ms (map-shuffled) and _mu (map-unshuffled).
Output files are named {config}_{ms|mu}.txt  →  displayed as shuffled/unshuffled.
"""

import argparse
import os
import subprocess
import sys
import warnings
from pathlib import Path

# DaCe's SymbolPropagation pass emits one ``UserWarning: loop_body maps
# to unused symbol(s): {<200 names>}`` per nested map per config. On the
# E8 sweep that's 50k+ chars of noise per binary that drowns the actual
# compile errors when you paste a log into chat. The warning is benign
# (unbound shape symbols carried by struct propagation; codegen emits
# them as placeholders). Suppress it here so the .out log is readable.
warnings.filterwarnings(
    "ignore",
    message=r".*maps to unused symbol\(s\):.*",
    category=UserWarning,
)
os.environ.setdefault("PYTHONWARNINGS",
                      "ignore:.*maps to unused symbol\\(s\\):UserWarning")

# Ensure DaCe is on f2dace/staging before any ``import dace`` fires.
# ``utils/__init__.py`` does the same, but doing it here too means a
# dirty-tree refusal surfaces at the top-level driver (clear error),
# not deep in a subprocess's import chain.
# Skipped under --dry-run / --list: enumerating configs from
# PERMUTE_CONFIGS doesn't touch DaCe, so the switch shouldn't block a
# login-node preview when the working tree is dirty.
if "--dry-run" not in sys.argv and "--list" not in sys.argv:
    import importlib.util as _ilu  # noqa: E402
    _branch_path = Path(__file__).resolve().parent / "utils" / "dace_branch.py"
    _spec = _ilu.spec_from_file_location("_e8_dace_branch", _branch_path)
    _dace_branch = _ilu.module_from_spec(_spec)
    _spec.loader.exec_module(_dace_branch)
    _dace_branch.ensure_branch(_dace_branch.F2DACE_BRANCH)

from sc26_layout.permute_stage8 import PERMUTE_CONFIGS  # noqa: E402

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

STAGE    = os.getenv("_STAGE", "8")
BEVERIN  = os.getenv("BEVERIN", "0") == "1"
LOWERED = os.getenv("_REDUCE_BITWIDTH_TRANSFORMATION", "0") == "1"
LOWERED_PREFIX = "lowered_" if LOWERED else ""
LOWERED_SUFFIX = "_lowered" if LOWERED else ""
COMPILE_CMD    = f"python -m utils.stages.compile_gpu_stage{STAGE}"


EXE_TEMPLATE   = f"./velocity_gpu{LOWERED_SUFFIX}.stage{STAGE}_standalone_release_permuted"
EXE_UNPERMUTED = f"./velocity_gpu{LOWERED_SUFFIX}.stage{STAGE}_standalone_release_unpermuted{LOWERED_SUFFIX}"
OUT_DIR        = Path(f"{'beverin_' if BEVERIN else 'daint_'}{LOWERED_PREFIX}full_permutations_{STAGE}")

# Internal suffix → human label mapping
_SHUFFLE_VARIANTS = [
    ("ms", "shuffled"),
    ("mu", "unshuffled"),
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def ensure_out_dir():
    OUT_DIR.mkdir(exist_ok=True)


def compile_config(name: str) -> bool:
    """Compile both ms and mu variants for *name* in one call.

    Pass ``--optimize`` and ``--compile`` so the optimization pipeline
    runs even when the user invokes the runner without flags. Release
    builds come from ``_RELEASE=1`` exported in setup_{daint,beverin}.sh
    -- compile_action() in utils/stages/common.py reads that env.

    The post-codegen reductions patcher (``tools/patch_codegen_reductions.py``)
    is wired into ``compile_action`` via ``post_codegen_hook``, so it
    runs inline between codegen and nvcc -- no retry-on-failure needed
    at this layer.
    """
    cmd = f"{COMPILE_CMD} --optimize --compile --permutations {name}"
    print(f"[compile] {cmd}")
    ret = subprocess.run(cmd, shell=True)
    if ret.returncode != 0:
        print(f"[compile] FAILED for {name} (rc={ret.returncode})", file=sys.stderr)
    return ret.returncode == 0


def _exe(name: str, suffix: str) -> str:
    """Return the executable path for config *name* and shuffle suffix *suffix*."""
    return EXE_TEMPLATE + f"_{name}_{suffix}{LOWERED_SUFFIX}"


def _out_file(name: str, label: str) -> Path:
    """Return the output txt path: {name}_{label}.txt  (label = shuffled|unshuffled)."""
    return OUT_DIR / f"{name}_{label}.txt"


def compile_unpermuted() -> bool:
    cmd = f"{COMPILE_CMD} --compile --unpermuted"
    print(f"[compile] {cmd}")
    ret = subprocess.run(cmd, shell=True)
    if ret.returncode != 0:
        print(f"[compile] FAILED for unpermuted (rc={ret.returncode})", file=sys.stderr)
    return ret.returncode == 0


def run_config(name: str, suffix: str, label: str, reps: int = 100, ncu: bool = False) -> bool:
    ensure_out_dir()
    exe = _exe(name, suffix)

    for step in [7, 9]:
        out_file = OUT_DIR / f"{name}_{label}_step{step}.txt"
        print(f"[run] {exe} step={step} -> {out_file}")
        with open(out_file, "w") as f:
            ret = subprocess.run(
                [exe, str(step), f"--reps={reps}"],
                stdout=f, stderr=subprocess.STDOUT,
            )
        if ret.returncode != 0:
            print(f"[run] FAILED for {name}/{label}/step{step} (rc={ret.returncode})",
                  file=sys.stderr)
            return False

        if ncu:
            ncu_report = OUT_DIR / f"ncu_{name}_{label}_step{step}"
            ncu_log    = OUT_DIR / f"ncu_{name}_{label}_step{step}.txt"
            print(f"[ncu] {name}/{label}/step{step} -> {ncu_report}.ncu-rep")
            ncu_cmd = [
                "ncu", "--set", "full",
                "--import-source", "yes",
                "--clock-control", "none",
                "-f", "-o", str(ncu_report),
                exe, str(step), f"--reps={reps}",
            ]
            with open(ncu_log, "w") as f:
                subprocess.run(ncu_cmd, stdout=f, stderr=subprocess.STDOUT)

    return True


def run_unpermuted(reps: int = 100, ncu: bool = False):
    ensure_out_dir()
    for step in [7, 9]:
        out_file = OUT_DIR / f"unpermuted_step{step}.txt"
        print(f"[run] {EXE_UNPERMUTED} step={step} -> {out_file}")
        with open(out_file, "w") as f:
            ret = subprocess.run(
                [EXE_UNPERMUTED, str(step), f"--reps={reps}"],
                stdout=f, stderr=subprocess.STDOUT,
            )
        if ret.returncode != 0:
            print(f"[run] unpermuted FAILED step={step} (rc={ret.returncode})",
                  file=sys.stderr)

        if ncu:
            ncu_report = OUT_DIR / f"ncu_unpermuted_step{step}"
            ncu_log    = OUT_DIR / f"ncu_unpermuted_step{step}.txt"
            ncu_cmd = [
                "ncu", "--set", "full",
                "--import-source", "yes",
                "--clock-control", "none",
                "-f", "-o", str(ncu_report),
                EXE_UNPERMUTED, str(step), f"--reps={reps}",
            ]
            with open(ncu_log, "w") as f:
                subprocess.run(ncu_cmd, stdout=f, stderr=subprocess.STDOUT)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    # Named aliases excluded from the default sweep
    _NAMED = {"index_only", "nlev_first"}
    all_names = [k for k in PERMUTE_CONFIGS if k not in _NAMED]

    # Curated default selection: nlev_first -> index_only -> unpermuted
    # (= winner_v1) -> empirical V_k cross-product cells from E6's
    # layout_crossproduct_winners.json (registered via v123_bridge as
    # v123_cv_V?_ch_V?_..._lm_V?). Mirrors E7's --v123-json default,
    # see Experiments/E7_FullVelocityTendencies/tools/run_layout_configs.py.
    _v123_names = sorted(k for k in PERMUTE_CONFIGS if k.startswith("v123_"))
    _CURATED_DEFAULT = (
        ["nlev_first", "index_only", "winner_v1"]
        + _v123_names
    )

    ap = argparse.ArgumentParser(description="Run stage-8 layout permutation sweep")
    ap.add_argument("--configs", type=str, default=None,
                    help="Comma-separated config names (default: all 95 sweep configs)")
    ap.add_argument("--unpermuted", action="store_true", default=False,
                    help="Also compile and run the unpermuted baseline")
    ap.add_argument("--reps",         type=int,  default=3)
    ap.add_argument("--ncu",          action="store_true", help="Run Nsight Compute")
    ap.add_argument("--compile-only", action="store_true")
    ap.add_argument("--run-only",     action="store_true")
    ap.add_argument("--dry-run",      action="store_true",
                    help="Print what would be compiled/run without doing it")
    ap.add_argument("--list",         action="store_true",
                    help="List all available config names and exit")
    args = ap.parse_args()

    if args.list:
        print("Sweep configs:")
        for name in all_names:
            if name in _NAMED:
                continue
            if "cv0" in name:
                continue
            pm = PERMUTE_CONFIGS[name]["permute_map"]
            print(f"  {name:40s}  ({len(pm):2d} arrays permuted)")
        print(f"\nNamed configs:")
        for name in _NAMED:
            if name not in PERMUTE_CONFIGS:
                continue
            if "cv0" in name:
                continue
            pm = PERMUTE_CONFIGS[name]["permute_map"]
            print(f"  {name:40s}  ({len(pm):2d} arrays permuted)")
        sweep_count = sum(1 for n in all_names if n not in _NAMED and "cv0" not in n)
        named_count = sum(1 for n in _NAMED if n in PERMUTE_CONFIGS and "cv0" not in n)
        print(f"\nSweep total : {sweep_count} configs")
        print(f"Named total : {named_count} configs")
        return

    # Resolve selection
    if args.configs:
        selected = []
        for c in args.configs.split(","):
            c = c.strip()
            if c not in PERMUTE_CONFIGS:
                print(f"Unknown config: {c}", file=sys.stderr)
                print(f"Use --list to see available configs", file=sys.stderr)
                sys.exit(1)
            selected.append(c)
    elif args.unpermuted:
        selected = []   # --unpermuted alone: skip sweep entirely
    else:
        # Default = curated (nlev_first, index_only, winner_v1, then all
        # v123_* registered by v123_bridge). Falls back to the legacy
        # 95-cell sweep if the v123 bridge produced nothing (e.g. E6
        # JSON missing). Use --configs to override explicitly.
        if any(n.startswith("v123_") for n in PERMUTE_CONFIGS):
            selected = [n for n in _CURATED_DEFAULT if n in PERMUTE_CONFIGS]
        else:
            selected = all_names


    # Summary
    banner = "[DRY RUN]  " if args.dry_run else ""
    print(f"{'=' * 60}")
    print(f"{banner}Layout permutation sweep: {len(selected)} configs  "
          f"({len(selected) * 2} executables)")
    if args.unpermuted:
        print(f"  + unpermuted baseline")
    print(f"Output directory: {OUT_DIR}/")
    print(f"{'=' * 60}")
    if args.dry_run:
        # Concise "what would run" listing: one row per config.
        for name in selected:
            pm = PERMUTE_CONFIGS[name]["permute_map"]
            print(f"  {name:40s}  ({len(pm):2d} arrays permuted)  "
                  f"-> {_out_file(name, 'shuffled').name}, "
                  f"{_out_file(name, 'unshuffled').name}")
        print()
        print(f"[DRY RUN] {len(selected)} configs * 2 (shuffled/unshuffled) "
              f"= {len(selected) * 2} binaries. No compile or execution.")
        return

    # Full plan (non-dry): per-binary executable + output paths.
    for name in selected:
        for suffix, label in _SHUFFLE_VARIANTS:
            exe      = _exe(name, suffix)
            out_file = _out_file(name, label)
            print(f"  {name:40s}  [{label:10s}]  exe={exe}  out={out_file}")
    print()

    ensure_out_dir()

    # Unpermuted baseline
    if args.unpermuted:
        print(f"\n{'=' * 60}")
        print(f"Unpermuted baseline")
        print(f"{'=' * 60}")
        if not args.run_only:
            compile_unpermuted()
        if not args.compile_only:
            run_unpermuted(reps=args.reps, ncu=args.ncu)

    # Each permuted config
    for name in selected:
        print(f"\n{'=' * 60}")
        print(f"Config: {name}")
        print(f"{'=' * 60}")

        if not args.run_only:
            # Check if both executables already exist
            both_exist = all(
                os.path.exists(_exe(name, suffix))
                for suffix, _ in _SHUFFLE_VARIANTS
            )
            if both_exist:
                print(f"[skip] {name}: both executables already exist")
            else:
                ok = compile_config(name)
                if not ok:
                    print(f"[skip] {name}: compile failed, skipping run")
                    continue

        if not args.compile_only:
            for suffix, label in _SHUFFLE_VARIANTS:
                run_config(name, suffix, label, reps=args.reps, ncu=args.ncu)

    # Final summary
    if not args.compile_only and not args.dry_run:
        print(f"\n{'=' * 60}")
        print(f"Done. Results in {OUT_DIR}/")
        for f in sorted(OUT_DIR.glob("*.txt")):
            print(f"  {f}")


if __name__ == "__main__":
    main()