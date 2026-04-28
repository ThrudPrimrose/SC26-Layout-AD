#!/usr/bin/env bash
# =============================================================================
# Per-experiment activation. Sourced from run_daint.sh / run_beverin.sh.
#
#   source <repo>/Experiments/common/activate.sh
#
# WHAT THIS SCRIPT DOES
#   1. Activates the venv at common/venv (manual: no bin/activate exists
#      because spack's CPython ships without venv script templates).
#   2. Ensures the DaCe clone is on the requested branch.
#
# WHAT THIS SCRIPT ASSUMES
#   * `common/setup.sh` has already been run (venv + DaCe clone exist).
#
# Env overrides:
#   DACE_BRANCH        default yakup/dev  (E8 sets f2dace/staging before sourcing)
#   DACE_DIR           default <common>/dace
#   VENV_DIR           default <common>/venv
#
# TODO: VERSION — if spack is re-concretized, update hashes here AND in
#   common/setup.sh in lockstep.
# =============================================================================

_COMMON_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${VENV_DIR:-${_COMMON_DIR}/venv}"
DACE_DIR="${DACE_DIR:-${_COMMON_DIR}/dace}"
DACE_BRANCH="${DACE_BRANCH:-yakup/dev}"

# Python source: system /usr/bin/python3.11 on both clusters. The spack
# python (python/asgm25z, 3.13.8 zen3) on Beverin starts up with
# ``LookupError: no codec search functions registered`` because the
# zen3-built stdlib doesn't match Beverin's filesystem layout, so the
# system python3.11 is the canonical interpreter for both. See
# setup.sh for the full rationale.
if [[ ! -x "${VENV_DIR}/bin/python" ]]; then
    echo "[activate] venv missing at ${VENV_DIR}. Run: bash ${_COMMON_DIR}/setup.sh" >&2
    return 1 2>/dev/null || exit 1
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
  # Sync submodules unconditionally: yakup/dev and f2dace/staging
  # may pin different submodule SHAs, and a hand-checkout outside
  # this script may leave them stale.
  git submodule update --init --recursive --quiet
  popd >/dev/null
else
  echo "[activate] WARNING: ${DACE_DIR} is not a git clone. Run setup.sh." >&2
fi

export DACE_DIR VENV_DIR DACE_BRANCH
