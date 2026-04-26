#!/usr/bin/env bash
# verify_baselines.sh -- compile the 4 baseline variants into a single
# binary (via ``compile_baselines.py``) and run it against committed
# reference data on a small set of timesteps. Confirms that f2dace ->
# generate_baselines -> compile_baselines -> link still produces a
# numerically-correct binary on the maintainer machine.
#
# Pipeline order:
#   1. tools/f90_to_sdfg.sh             baseline/velocity_no_nproma.sdfgz   (f2dace/staging)
#   2. python generate_baselines.py     4 specialised variants              (f2dace/staging)
#   3. tools/verify_baselines.sh        THIS                                (yakup/dev or f2dace/staging)
#   4. python -m utils.stages.stage1    stage 1 SDFGs                       (yakup/dev)
#   5. tools/verify_stage1.sh           stage 1 numerical check             (yakup/dev)
#
# Env knobs:
#   PYTHON              python interpreter (default: python3)
#   TIMESTEPS           comma list (default: 1,7,9)
#   REPS                main.cpp --reps (default: 5)
#   DATA                data root  (default: data_r02b05)
#   OUTPUT_BIN          binary name (default: velocity_baseline)
#   OUTPUT_DIR          gotwant dir (default: gotwant/baseline)

set -euo pipefail

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON="${PYTHON:-python3}"
TIMESTEPS="${TIMESTEPS:-1,7,9}"
REPS="${REPS:-5}"
DATA="${DATA:-${EXP_DIR}/data_r02b05}"
OUTPUT_BIN="${OUTPUT_BIN:-velocity_baseline}"
OUTPUT_DIR="${OUTPUT_DIR:-${EXP_DIR}/gotwant/baseline}"

cd "${EXP_DIR}"

# Prerequisite: 4 variants present.
for v in 0_istep_1 0_istep_2 1_istep_1 1_istep_2; do
  f="baseline/velocity_no_nproma_if_prop_lvn_only_${v}.sdfgz"
  if [[ ! -f "$f" ]]; then
    echo "[verify_baselines] missing $f. Run python generate_baselines.py first." >&2
    exit 1
  fi
done

echo "[verify_baselines] compiling baseline variants -> ${OUTPUT_BIN}"
"${PYTHON}" compile_baselines.py --output "${OUTPUT_BIN}"

if [[ ! -x "${EXP_DIR}/${OUTPUT_BIN}" ]]; then
  echo "[verify_baselines] error: ${OUTPUT_BIN} did not build" >&2
  exit 2
fi

mkdir -p "${OUTPUT_DIR}"
echo "[verify_baselines] running on timesteps ${TIMESTEPS}, reps=${REPS}"
"${EXP_DIR}/${OUTPUT_BIN}" \
  --data "${DATA}" \
  --reps "${REPS}" \
  --timesteps "${TIMESTEPS}" \
  --output-dir "${OUTPUT_DIR}"

echo
echo "[verify_baselines] gotwant dir: ${OUTPUT_DIR}"
echo "[verify_baselines] ok. Next: python -m utils.stages.stage1 --optimize --compile  (yakup/dev)"
