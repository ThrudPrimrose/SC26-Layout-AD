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
#   3. Clones DaCe into common/dace on branch yakup/dev. E1..E6 don't
#      import dace at all; E7 stays on yakup/dev; E8 swaps to
#      f2dace/staging at run time via DACE_BRANCH (run_*.sh exports it
#      before sourcing activate.sh, which is opt-in on that variable).
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
#   DACE_BRANCH    default yakup/dev (E8 switches to f2dace/staging at run time)
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

# set -euo pipefail  # disabled to surface failing commands without exiting

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${VENV_DIR:-${SCRIPT_DIR}/venv}"
DACE_DIR="${DACE_DIR:-${SCRIPT_DIR}/dace}"
DACE_URL="${DACE_URL:-https://github.com/spcl/dace.git}"
DACE_BRANCH="${DACE_BRANCH:-yakup/dev}"

# Python source: system /usr/bin/python3.11 on both clusters.
#  * On Daint (aarch64) — historical default; spack scratch installs there
#    get purged by the Capstor atime-based cleanup, so the system python
#    is more durable.
#  * On Beverin (x86_64) — the spack python (python/asgm25z, 3.13.8 zen3)
#    fails to start with ``LookupError: no codec search functions
#    registered`` because its compiled stdlib path doesn't match the
#    Beverin login-node filesystem layout (probably a cross-build leak
#    via spack's binary-cache fetch). System python3.11 is verified
#    working: ``$ python3.11 -V → Python 3.11.11``.
# Override with SC26_PYBIN=/path/to/python to force a specific interpreter.
log() { printf '[setup] %s\n' "$*"; }

# Resolve a python3.11 interpreter in priority order:
#   1. ``${SC26_PYBIN}`` if explicitly set
#   2. ``/usr/bin/python3.11`` (system; the canonical AD path)
#   3. any ``python3.11`` on PATH (e.g. pyenv-shimmed)
#   4. install via pyenv if available, else error
PYBIN="${SC26_PYBIN:-}"
if [[ -z "${PYBIN}" || ! -x "${PYBIN}" ]]; then
    if [[ -x /usr/bin/python3.11 ]]; then
        PYBIN=/usr/bin/python3.11
    elif command -v python3.11 >/dev/null 2>&1; then
        PYBIN="$(command -v python3.11)"
    elif command -v pyenv >/dev/null 2>&1; then
        log "python3.11 not found; installing via pyenv (this may take a few minutes)"
        # ``pyenv install --skip-existing`` is idempotent. Pick the
        # newest 3.11.x pyenv knows about.
        _PY311_VER="$(pyenv install --list 2>/dev/null | awk '/^ *3\.11\.[0-9]+$/{print $1}' | tail -1)"
        if [[ -z "${_PY311_VER}" ]]; then
            log "ERROR: pyenv has no 3.11.x available; run \`pyenv update\` first"
            exit 1
        fi
        pyenv install --skip-existing "${_PY311_VER}"
        PYBIN="$(pyenv root)/versions/${_PY311_VER}/bin/python3.11"
        unset _PY311_VER
        if [[ ! -x "${PYBIN}" ]]; then
            log "ERROR: pyenv install completed but ${PYBIN} is not executable"
            exit 1
        fi
    else
        log "ERROR: no python3.11 found. Install one of:"
        log "         (a) system /usr/bin/python3.11 (apt install python3.11 / equivalent)"
        log "         (b) pyenv (https://github.com/pyenv/pyenv) — setup.sh will then \`pyenv install\`"
        log "         (c) override with SC26_PYBIN=/path/to/python3.11"
        exit 1
    fi
fi
if [[ ! -x "${PYBIN}" ]]; then
    log "ERROR: resolved interpreter ${PYBIN} is not executable"
    exit 1
fi

log "using $("${PYBIN}" --version) at ${PYBIN}"

# Sanity-check that stdlib C-exts actually import.
"${PYBIN}" - <<'PY'
import sqlite3, readline, bz2, lzma, ctypes, ssl, zlib  # noqa: F401
PY

# --- venv ----------------------------------------------------------------
# NOTE: spack's CPython ships with empty venv/scripts/{common,posix} template
# dirs and a pip wheel missing from ensurepip/_bundled. So `python -m venv`
# produces a working interpreter tree but no `bin/activate` and no pip.
# We work around both: create with --without-pip, bootstrap pip via
# get-pip.py, and activate by exporting VIRTUAL_ENV + PATH manually.
# Detect a stale venv (e.g. one created with an older interpreter that
# no longer exists / fails to start) and rebuild it from scratch. The
# canonical signal: venv/bin/python should start successfully AND its
# sys._base_executable should resolve to the same realpath as PYBIN.
# If either check fails, blow away the venv directory.
_venv_ok=0
if [[ -x "${VENV_DIR}/bin/python" ]]; then
    if "${VENV_DIR}/bin/python" -c "
import sys, os
want = os.path.realpath('${PYBIN}')
got  = os.path.realpath(sys._base_executable)
sys.exit(0 if want == got else 1)
" >/dev/null 2>&1; then
        _venv_ok=1
    else
        log "venv at ${VENV_DIR} is stale or broken (different base interpreter); rebuilding"
        rm -rf "${VENV_DIR}"
    fi
fi
if (( _venv_ok == 0 )); then
    log "creating venv at ${VENV_DIR}"
    "${PYBIN}" -m venv --without-pip "${VENV_DIR}"
fi
unset _venv_ok

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

# Make ``python`` resolve to the venv interpreter for the caller's
# shell, bypassing pyenv shims that might intercept on PATH. Aliases
# set inside a sourced script propagate to the caller. ``shopt -s
# expand_aliases`` lets the alias also expand in subshells that source
# this script non-interactively (e.g. some sbatch wrappers).
shopt -s expand_aliases 2>/dev/null || true
alias python="${VENV_DIR}/bin/python"
alias python3="${VENV_DIR}/bin/python"

log "done."
log "activate later with:  source ${SCRIPT_DIR}/activate.sh"
log "  (\`python\` is aliased to ${VENV_DIR}/bin/python in this shell)"
