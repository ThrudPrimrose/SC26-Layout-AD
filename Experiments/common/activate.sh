#!/usr/bin/env bash
# =============================================================================
# Per-experiment activation. Sourced from run_daint.sh / run_beverin.sh.
#
#   source <repo>/Experiments/common/activate.sh
#
# WHAT THIS SCRIPT DOES
#   1. Re-loads the same spack python + sqlite that setup.sh used, so the
#      venv's shebang resolves and `import sqlite3` works. readline / bz2 /
#      lzma / ctypes / ssl / zlib come for free via the python's RPATH.
#   2. Activates the venv at common/venv (manual: no bin/activate exists
#      because spack's CPython ships without venv script templates).
#   3. Ensures the DaCe clone is on the requested branch.
#
# WHAT THIS SCRIPT ASSUMES
#   * `common/setup.sh` has already been run (venv + DaCe clone exist).
#   * `spack` is on PATH and the short-hash specs still resolve.
#
# Env overrides:
#   SC26_PYTHON_SPEC   auto-selected by arch; override to force
#   DACE_BRANCH        default yakup/dev  (e.g. f2dace/staging for E6 full module)
#   DACE_DIR           default <common>/dace
#   VENV_DIR           default <common>/venv
#
# NOTE: we use SC26_PYTHON_SPEC (not SPACK_PYTHON) because spack itself
# sets SPACK_PYTHON internally to its bootstrap interpreter.
#
# Arch auto-selection — must match common/setup.sh:
#   x86_64  (beverin, zen3)      -> python/asgm25z + zen3 prereq hashes
#   aarch64 (daint, neoverse_v2) -> python/6kewgi6 + neoverse_v2 prereq hashes
#
# TODO: VERSION — if spack is re-concretized, update hashes here AND in
#   common/setup.sh in lockstep.
# =============================================================================

_COMMON_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${VENV_DIR:-${_COMMON_DIR}/venv}"
DACE_DIR="${DACE_DIR:-${_COMMON_DIR}/dace}"
DACE_BRANCH="${DACE_BRANCH:-yakup/dev}"

# Python source: spack on beverin (zen3), system python3.11 on daint
# (aarch64). See setup.sh for the rationale (Capstor scratch purge).
case "$(uname -m)" in
  x86_64)
    : "${SC26_PYTHON_SPEC:=python/asgm25z}"   # python@3.13.8, zen3
    SPACK_DEPS=(
        sqlite/atf6liaa     # sqlite@3.50.4
    )
    USE_SPACK_PYTHON=1
    ;;
  aarch64)
    USE_SPACK_PYTHON=0
    ;;
  *)
    echo "[activate] ERROR: unsupported arch $(uname -m)." >&2
    return 1 2>/dev/null || exit 1
    ;;
esac

if [[ ! -x "${VENV_DIR}/bin/python" ]]; then
  echo "[activate] venv missing at ${VENV_DIR}. Run: bash ${_COMMON_DIR}/setup.sh" >&2
  return 1 2>/dev/null || exit 1
fi

if (( USE_SPACK_PYTHON )); then
  if ! command -v spack >/dev/null 2>&1; then
    echo "[activate] ERROR: spack not found on PATH." >&2
    return 1 2>/dev/null || exit 1
  fi
  spack load "${SPACK_DEPS[@]}"
  spack load "${SC26_PYTHON_SPEC}"
fi

# Manual venv activation (spack's CPython lacks bin/activate; we use the
# same code path on daint for symmetry).
export VIRTUAL_ENV="${VENV_DIR}"
export PATH="${VENV_DIR}/bin:${PATH}"
unset PYTHONHOME
hash -r 2>/dev/null

if [[ -d "${DACE_DIR}/.git" ]]; then
  pushd "${DACE_DIR}" >/dev/null
  CURRENT_BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo '')"
  if [[ "${CURRENT_BRANCH}" != "${DACE_BRANCH}" ]]; then
    echo "[activate] switching DaCe ${CURRENT_BRANCH:-<detached>} -> ${DACE_BRANCH}"
    git fetch origin --quiet
    git checkout "${DACE_BRANCH}"
  fi
  popd >/dev/null
else
  echo "[activate] WARNING: ${DACE_DIR} is not a git clone. Run setup.sh." >&2
fi

export DACE_DIR VENV_DIR DACE_BRANCH SC26_PYTHON_SPEC
