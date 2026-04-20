#!/usr/bin/env bash
# One-time global setup for the SC26 layout artifact.
#
# Installs Python 3.13 via pyenv (if missing), creates a venv at
# common/venv, clones DaCe into common/dace on branch yakup/dev, and
# installs DaCe + plotting dependencies in editable mode.
#
# Safe to re-run: each step is idempotent.
#
# Env overrides:
#   PYTHON_VERSION  default 3.13.1
#   DACE_URL        default https://github.com/spcl/dace.git
#   DACE_BRANCH     default yakup/dev
#   DACE_DIR        default <common>/dace
#   VENV_DIR        default <common>/venv

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_VERSION="${PYTHON_VERSION:-3.13.1}"
VENV_DIR="${VENV_DIR:-${SCRIPT_DIR}/venv}"
DACE_DIR="${DACE_DIR:-${SCRIPT_DIR}/dace}"
DACE_URL="${DACE_URL:-https://github.com/spcl/dace.git}"
DACE_BRANCH="${DACE_BRANCH:-yakup/dev}"

log() { printf '[setup] %s\n' "$*"; }

# --- pyenv ---------------------------------------------------------------
if ! command -v pyenv >/dev/null 2>&1; then
  log "ERROR: pyenv not found in PATH. Install pyenv and re-run."
  exit 1
fi
export PYENV_ROOT="${PYENV_ROOT:-$(pyenv root)}"
eval "$(pyenv init -)"

if ! pyenv versions --bare | grep -qx "${PYTHON_VERSION}"; then
  log "installing Python ${PYTHON_VERSION} via pyenv"
  pyenv install -s "${PYTHON_VERSION}"
fi
PYBIN="${PYENV_ROOT}/versions/${PYTHON_VERSION}/bin/python"
log "using ${PYBIN}"

# --- venv ----------------------------------------------------------------
if [[ ! -x "${VENV_DIR}/bin/python" ]]; then
  log "creating venv at ${VENV_DIR}"
  "${PYBIN}" -m venv "${VENV_DIR}"
fi
source "${VENV_DIR}/bin/activate"
python -m pip install --upgrade pip wheel setuptools

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
pip install -e .
cd "${SCRIPT_DIR}"

# --- plotting / analysis deps -------------------------------------------
pip install numpy scipy matplotlib pandas

log "done."
log "activate later with:  source ${SCRIPT_DIR}/activate.sh"
