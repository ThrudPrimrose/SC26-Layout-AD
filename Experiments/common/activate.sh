#!/usr/bin/env bash
# =============================================================================
# Per-experiment activation. Sourced from run_daint.sh / run_beverin.sh.
#
#   source <repo>/Experiments/common/activate.sh
#
# WHAT THIS SCRIPT DOES
#   1. Activates the venv at common/venv (manual: no bin/activate exists
#      because spack's CPython ships without venv script templates).
#   2. (DaCe-using experiments only) If ``DACE_BRANCH`` is explicitly
#      exported by the caller, ensures the DaCe clone is on that branch.
#      No default — E1..E6 never set ``DACE_BRANCH`` and therefore never
#      trigger a branch switch. E7 pins ``yakup/dev`` and E8 pins
#      ``f2dace/staging`` before sourcing this script. Do NOT run E7 and
#      E8 concurrently against the same DaCe clone -- they would fight
#      over HEAD; use a separate ``$DACE_DIR`` per experiment if needed.
#
# WHAT THIS SCRIPT ASSUMES
#   * `common/setup.sh` has already been run (venv exists; the DaCe
#     clone exists too if any experiment in the run needs it).
#
# Env overrides:
#   DACE_BRANCH        unset by default; export = explicit pin
#                      (E7 -> yakup/dev, E8 -> f2dace/staging)
#   DACE_DIR           default <common>/dace
#   VENV_DIR           default <common>/venv
#
# TODO: VERSION — if spack is re-concretized, update hashes here AND in
#   common/setup.sh in lockstep.
# =============================================================================

_COMMON_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${VENV_DIR:-${_COMMON_DIR}/venv}"
DACE_DIR="${DACE_DIR:-${_COMMON_DIR}/dace}"
# DACE_BRANCH is OPT-IN: no default. E1..E6 never set it, so no branch
# switch ever happens for them. E7 sets ``yakup/dev``; E8 sets
# ``f2dace/staging``. Both pin before sourcing this script.

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

if [[ -n "${DACE_BRANCH:-}" ]]; then
  if [[ -d "${DACE_DIR}/.git" ]]; then
    pushd "${DACE_DIR}" >/dev/null
    CURRENT_BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo '')"
    if [[ "${CURRENT_BRANCH}" != "${DACE_BRANCH}" ]]; then
      echo "[activate] switching DaCe ${CURRENT_BRANCH:-<detached>} -> ${DACE_BRANCH}"
      git fetch origin --quiet
      git checkout "${DACE_BRANCH}"
    fi
    # Sync submodules: a hand-checkout outside this script may leave
    # the recorded submodule SHAs stale.
    git submodule update --init --recursive --quiet
    popd >/dev/null
  else
    echo "[activate] WARNING: DACE_BRANCH=${DACE_BRANCH} requested but ${DACE_DIR} is not a git clone. Run setup.sh." >&2
  fi
  export DACE_BRANCH
fi

export DACE_DIR VENV_DIR

# Make ``python`` resolve to the venv interpreter for the caller's
# shell. PATH already starts with ``${VENV_DIR}/bin``, but pyenv shims
# (when installed in $HOME) often re-inject themselves at lookup time.
# An explicit alias bypasses that. ``shopt -s expand_aliases`` keeps the
# alias active inside non-interactive subshells (sbatch wrappers, etc.)
# that source this script.
shopt -s expand_aliases 2>/dev/null || true
alias python="${VENV_DIR}/bin/python"
alias python3="${VENV_DIR}/bin/python"
