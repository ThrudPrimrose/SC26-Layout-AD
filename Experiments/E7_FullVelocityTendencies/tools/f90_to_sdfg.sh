#!/usr/bin/env bash
# f90_to_sdfg.sh -- the F90 -> first-SDFG entry point for E7.
#
# Wraps tools/sdfg_from_velocity_f90.py with the prerequisite checks the
# Python script doesn't perform itself: that DaCe is on a branch shipping
# the f2dace Fortran frontend (typically f2dace/staging) and that all
# DaCe submodules are initialized.
#
# Output: baseline/velocity_no_nproma.sdfgz -- the input expected by the
# next step in the pipeline (generate_baselines.py).
#
# Pipeline order:
#   1. tools/f90_to_sdfg.sh             this script           (f2dace/staging)
#   2. python generate_baselines.py     4 specialised variants (f2dace/staging)
#   3. python compile_baselines.py      compile + link (also gives main binary)
#   4. tools/verify_baselines.sh        run binary on TS {1,7,9}
#   5. python -m utils.stages.stage1    optimize + compile        (yakup/dev)
#   6. tools/verify_stage1.sh           re-run binary, recheck    (yakup/dev)
#
# Env knobs:
#   PYTHON              python interpreter (default: python3)
#   INPUT               .f90 source (default: baseline_inputs/velocity_modified.f90)
#   OUTPUT              .sdfgz output (default: baseline/velocity_no_nproma.sdfgz)
#   DACE_DIR            DaCe checkout to verify Fortran frontend in
#                       (default: $HOME/Work/dace; only used for the prereq check)

set -euo pipefail

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON="${PYTHON:-python3}"
INPUT="${INPUT:-${EXP_DIR}/baseline_inputs/velocity_modified.f90}"
OUTPUT="${OUTPUT:-${EXP_DIR}/baseline/velocity_no_nproma.sdfgz}"
DACE_DIR="${DACE_DIR:-${HOME}/Work/dace}"

cd "${EXP_DIR}"

# Prerequisite 1: input exists.
if [[ ! -f "${INPUT}" ]]; then
    echo "[f90_to_sdfg] error: input not found: ${INPUT}" >&2
    exit 1
fi

# Prerequisite 2: f2dace frontend is importable. We skip the
# branch-name check (it's the caller's responsibility to be on
# f2dace/staging or any branch that ships the frontend) but verify the
# concrete symbol the script needs.
if ! "${PYTHON}" -c "from dace.frontend.fortran.fortran_parser import ParseConfig" 2>/dev/null; then
    echo "[f90_to_sdfg] error: 'ParseConfig' not in dace.frontend.fortran.fortran_parser." >&2
    echo "[f90_to_sdfg] DaCe must be on a branch shipping the f2dace frontend (typically f2dace/staging)." >&2
    if [[ -d "${DACE_DIR}/.git" ]]; then
        cur=$(git -C "${DACE_DIR}" rev-parse --abbrev-ref HEAD 2>/dev/null || echo '?')
        echo "[f90_to_sdfg] current DaCe branch: ${cur} (${DACE_DIR})" >&2
    fi
    exit 1
fi

# Prerequisite 3: DaCe submodules. The codegen path imports
# dace.codegen.targets.xilinx which depends on dace.external.rtllib.templates;
# unpopulated submodules raise a confusing ModuleNotFoundError mid-run.
if [[ -d "${DACE_DIR}/.git" ]]; then
    if ! "${PYTHON}" -c "import dace.external.rtllib.templates" 2>/dev/null; then
        echo "[f90_to_sdfg] DaCe submodules not initialized; running 'git submodule update --init' under ${DACE_DIR}"
        git -C "${DACE_DIR}" submodule update --init --recursive
    fi
fi

mkdir -p "$(dirname "${OUTPUT}")"

echo "[f90_to_sdfg] input  : ${INPUT}"
echo "[f90_to_sdfg] output : ${OUTPUT}"
"${PYTHON}" tools/sdfg_from_velocity_f90.py --input "${INPUT}" --output "${OUTPUT}"

if [[ ! -f "${OUTPUT}" ]]; then
    echo "[f90_to_sdfg] error: f2dace did not produce ${OUTPUT}" >&2
    exit 2
fi

echo "[f90_to_sdfg] ok. Next: python generate_baselines.py --input ${OUTPUT##${EXP_DIR}/}"
