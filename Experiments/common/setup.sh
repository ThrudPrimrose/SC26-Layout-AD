#!/usr/bin/env bash
# =============================================================================
# One-time global setup for the SC26 layout artifact.
#
# WHAT THIS SCRIPT DOES
#   1. spack-loads a pinned CPython plus sqlite. Other stdlib C-ext
#      prereqs (readline, bz2, lzma, ctypes, ssl, zlib) are already
#      baked into the spack python via RPATH, so only sqlite needs to
#      be on the loader path.
#   2. Creates a venv at common/venv using that spack python.
#   3. Clones DaCe into common/dace on branch yakup/dev.
#   4. pip-installs DaCe (editable) + numpy / scipy / matplotlib / pandas.
#
# WHAT THIS SCRIPT ASSUMES
#   * `spack` is on PATH and the short-hash specs below resolve.
#     Default hashes are the linux-sles15-zen3 (beverin) builds. On daint
#     (linux-sles15-neoverse_v2) override SC26_PYTHON_SPEC=python/6kewgi6 and
#     the other SPACK_* env vars, or use the daint-specific deps.
#   * Outbound HTTPS to github.com + pypi.org for DaCe + pip deps.
#
# Safe to re-run: each step is idempotent.
#
# Env overrides:
#   SC26_PYTHON_SPEC   auto-selected by arch (see below); override to force
#   DACE_URL       default https://github.com/spcl/dace.git
#   DACE_BRANCH    default yakup/dev
#   DACE_DIR       default <common>/dace
#   VENV_DIR       default <common>/venv
#
# Arch auto-selection:
#   x86_64  (beverin, zen3)          -> python/asgm25z + zen3 prereq hashes
#   aarch64 (daint, neoverse_v2)     -> python/6kewgi6 + neoverse_v2 prereq hashes
#
# TODO: VERSION — short hashes below are pinned to a specific spack
#   concretization. Regenerate with `spack find -L <pkg> target=zen3`
#   (or `target=neoverse_v2`) if the spack env is re-concretized.
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${VENV_DIR:-${SCRIPT_DIR}/venv}"
DACE_DIR="${DACE_DIR:-${SCRIPT_DIR}/dace}"
DACE_URL="${DACE_URL:-https://github.com/spcl/dace.git}"
DACE_BRANCH="${DACE_BRANCH:-yakup/dev}"

# Spack prereqs for Python stdlib C-ext modules (readline, sqlite3, lzma,
# bz2, ctypes, ssl, zlib). Pinned by short hash — TODO: VERSION.
# Arch-specific: hashes differ between zen3 (beverin) and neoverse_v2 (daint).
case "$(uname -m)" in
  x86_64)
    : "${SC26_PYTHON_SPEC:=python/asgm25z}"   # python@3.13.8, zen3
    SPACK_DEPS=(
        sqlite/atf6liaa     # sqlite@3.50.4
    )
    ;;
  aarch64)
    : "${SC26_PYTHON_SPEC:=python/6kewgi6}"   # python@3.13.8, neoverse_v2
    SPACK_DEPS=(
        sqlite/aynz7gpz     # sqlite@3.50.4
    )
    ;;
  *)
    echo "[setup] ERROR: unsupported arch $(uname -m); set SC26_PYTHON_SPEC + SPACK_DEPS manually." >&2
    exit 1
    ;;
esac

log() { printf '[setup] %s\n' "$*"; }

# --- spack-provided python -----------------------------------------------
if ! command -v spack >/dev/null 2>&1; then
  log "ERROR: spack not found on PATH."
  exit 1
fi

log "loading spack prereqs (${SPACK_DEPS[*]})"
spack load "${SPACK_DEPS[@]}"

log "loading spack python (${SC26_PYTHON_SPEC})"
spack load "${SC26_PYTHON_SPEC}"

PYBIN="$(command -v python3)"
log "using $(python3 --version) at ${PYBIN}"

# Sanity-check that stdlib C-exts actually import.
python3 - <<'PY'
import sqlite3, readline, bz2, lzma, ctypes, ssl, zlib  # noqa: F401
PY

# --- venv ----------------------------------------------------------------
# NOTE: spack's CPython ships with empty venv/scripts/{common,posix} template
# dirs and a pip wheel missing from ensurepip/_bundled. So `python -m venv`
# produces a working interpreter tree but no `bin/activate` and no pip.
# We work around both: create with --without-pip, bootstrap pip via
# get-pip.py, and activate by exporting VIRTUAL_ENV + PATH manually.
if [[ ! -x "${VENV_DIR}/bin/python" ]]; then
  log "creating venv at ${VENV_DIR}"
  "${PYBIN}" -m venv --without-pip "${VENV_DIR}"
fi

# Manual activation (no bin/activate exists on this spack python).
export VIRTUAL_ENV="${VENV_DIR}"
export PATH="${VENV_DIR}/bin:${PATH}"
unset PYTHONHOME

if ! "${VENV_DIR}/bin/python" -m pip --version >/dev/null 2>&1; then
  log "bootstrapping pip inside venv (get-pip.py)"
  curl -fsSL https://bootstrap.pypa.io/get-pip.py -o "${SCRIPT_DIR}/.get-pip.py"
  "${VENV_DIR}/bin/python" "${SCRIPT_DIR}/.get-pip.py"
  rm -f "${SCRIPT_DIR}/.get-pip.py"
fi
"${VENV_DIR}/bin/python" -m pip install --upgrade pip wheel setuptools

# --- DaCe ----------------------------------------------------------------
if [[ ! -d "${DACE_DIR}/.git" ]]; then
  log "cloning DaCe into ${DACE_DIR}"
  git clone --recurse-submodules "${DACE_URL}" "${DACE_DIR}"
fi
cd "${DACE_DIR}"
git fetch origin --tags
git checkout "${DACE_BRANCH}"
git pull --ff-only origin "${DACE_BRANCH}" || log "pull skipped (detached or diverged)"
git submodule update --init --recursive
log "installing DaCe (editable) on branch ${DACE_BRANCH}"
"${VENV_DIR}/bin/python" -m pip install -e .
cd "${SCRIPT_DIR}"

# --- plotting / analysis deps -------------------------------------------
"${VENV_DIR}/bin/python" -m pip install numpy scipy matplotlib pandas

log "done."
log "activate later with:  source ${SCRIPT_DIR}/activate.sh"
