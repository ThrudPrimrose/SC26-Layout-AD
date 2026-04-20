#!/usr/bin/env bash
# Per-experiment activation. Sourced from run_daint.sh / run_beverin.sh.
#
#   source <repo>/Experiments/common/activate.sh
#
# Activates the pyenv venv created by setup.sh and ensures the DaCe
# clone is on the requested branch. Fails loudly if setup.sh has not
# been run yet.
#
# Env overrides:
#   DACE_BRANCH   default yakup/dev  (e.g. f2dace/staging for E6 full module)
#   DACE_DIR      default <common>/dace
#   VENV_DIR      default <common>/venv

_COMMON_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${VENV_DIR:-${_COMMON_DIR}/venv}"
DACE_DIR="${DACE_DIR:-${_COMMON_DIR}/dace}"
DACE_BRANCH="${DACE_BRANCH:-yakup/dev}"

if [[ ! -x "${VENV_DIR}/bin/python" ]]; then
  echo "[activate] venv missing at ${VENV_DIR}. Run: bash ${_COMMON_DIR}/setup.sh" >&2
  return 1 2>/dev/null || exit 1
fi

if command -v pyenv >/dev/null 2>&1; then
  export PYENV_ROOT="${PYENV_ROOT:-$(pyenv root)}"
  eval "$(pyenv init -)"
fi
source "${VENV_DIR}/bin/activate"

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

export DACE_DIR VENV_DIR DACE_BRANCH
