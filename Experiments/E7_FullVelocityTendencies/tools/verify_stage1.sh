#!/usr/bin/env bash
# verify_stage1.sh -- compile stage 1's optimised SDFGs into a single
# binary and run it against committed reference data on a small set of
# timesteps. Confirms that yakup/dev's stage 1 didn't break the frozen
# ABI from phase 1.
#
# Env knobs:
#   PYTHON              python interpreter (default: python3)
#   TIMESTEPS           comma list (default: 1,7,9)
#   REPS                main.cpp --reps (default: 5)
#   DATA                data root  (default: data_r02b05)
#   OUTPUT_BIN          binary name (default: velocity_stage1)
#   OUTPUT_DIR          gotwant dir (default: gotwant/stage1)

set -euo pipefail

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON="${PYTHON:-python3}"
TIMESTEPS="${TIMESTEPS:-1,7,9}"
REPS="${REPS:-5}"
DATA="${DATA:-${EXP_DIR}/data_r02b05}"
OUTPUT_BIN="${OUTPUT_BIN:-velocity_stage1}"
OUTPUT_DIR="${OUTPUT_DIR:-${EXP_DIR}/gotwant/stage1}"

cd "${EXP_DIR}"

# Prerequisite: stage 1 SDFGs present.
for v in 0_istep_1 0_istep_2 1_istep_1 1_istep_2; do
  f="codegen/stage1/velocity_no_nproma_if_prop_lvn_only_${v}.sdfgz"
  if [[ ! -f "$f" ]]; then
    echo "[verify_stage1] missing $f. Run python -m utils.stages.stage1 --optimize first." >&2
    exit 1
  fi
done

echo "[verify_stage1] compiling stage 1 SDFGs -> ${OUTPUT_BIN}"
"${PYTHON}" -m utils.stages.stage1 --no-optimize --compile

if [[ ! -x "${EXP_DIR}/${OUTPUT_BIN}" ]]; then
  echo "[verify_stage1] error: ${OUTPUT_BIN} did not build" >&2
  exit 2
fi

mkdir -p "${OUTPUT_DIR}"
echo "[verify_stage1] running on timesteps ${TIMESTEPS}, reps=${REPS}"
"${EXP_DIR}/${OUTPUT_BIN}" \
  --data "${DATA}" \
  --reps "${REPS}" \
  --timesteps "${TIMESTEPS}" \
  --output-dir "${OUTPUT_DIR}"

echo
echo "[verify_stage1] gotwant dir: ${OUTPUT_DIR}"
echo "[verify_stage1] ok."
