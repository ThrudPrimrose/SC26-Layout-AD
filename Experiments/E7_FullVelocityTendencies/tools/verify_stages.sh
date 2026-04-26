#!/usr/bin/env bash
# verify_stages.sh -- end-to-end build+run+gotwant for stages 1-4.
#
# For each stage in $STAGES:
#   1. python -m utils.stages.stage<N>          (--optimize --compile)
#   2. velocity_stage<N> --timesteps $TIMESTEPS --reps $REPS
#                        --output-dir gotwant/stage<N>
#   3. python utils/compare_got_and_want.py -r gotwant/stage<N>
#                                           -t $TIMESTEPS
#
# Default thread count is 4 (kernel + serde + comparator). Override via:
#   THREADS=8 ./tools/verify_stages.sh
# or finer:
#   OMP_NUM_THREADS=8 SLURM_CPUS_PER_TASK=8 ./tools/verify_stages.sh
#
# Other env knobs:
#   PYTHON       python interpreter (default: python3)
#   STAGES       space list (default: "1 2 3 4")
#   TIMESTEPS    comma list (default: 1,2)
#   REPS         reps per __program_ call (default: 1)
#   DATA         data root (default: ./data_r02b05)
#   GOTWANT_DIR  base gotwant dir (default: ./gotwant)

set -euo pipefail

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON="${PYTHON:-python3}"
STAGES="${STAGES:-1 2 3 4}"
TIMESTEPS="${TIMESTEPS:-1,2}"
REPS="${REPS:-1}"
DATA="${DATA:-${EXP_DIR}/data_r02b05}"
GOTWANT_DIR="${GOTWANT_DIR:-${EXP_DIR}/gotwant}"

# Single knob threading. ``THREADS`` is the parent setting; the three
# downstream env vars are derived unless the caller has set them
# explicitly. Default 4.
THREADS="${THREADS:-4}"
export OMP_NUM_THREADS="${OMP_NUM_THREADS:-${THREADS}}"
export SLURM_CPUS_PER_TASK="${SLURM_CPUS_PER_TASK:-${THREADS}}"
# main.cpp uses jthreads on demand; no global cap. The two env vars
# above cover the kernel (OMP) and the comparator (compare_got_and_want
# reads SLURM_CPUS_PER_TASK as DEFAULT_WORKERS). If the binary ever
# grows a --threads flag, add it to the run line below.

cd "${EXP_DIR}"

if [[ ! -d "${DATA}" ]]; then
    echo "[verify_stages] data root missing: ${DATA}" >&2
    echo "[verify_stages] run tools/download_data.sh first." >&2
    exit 1
fi

mkdir -p "${GOTWANT_DIR}"

echo "[verify_stages] stages   : ${STAGES}"
echo "[verify_stages] timesteps: ${TIMESTEPS}"
echo "[verify_stages] reps     : ${REPS}"
echo "[verify_stages] threads  : ${THREADS} (OMP=${OMP_NUM_THREADS}, "\
                                "SLURM=${SLURM_CPUS_PER_TASK})"
echo "[verify_stages] data     : ${DATA}"
echo "[verify_stages] gotwant  : ${GOTWANT_DIR}"
echo

fail=0
for stage in ${STAGES}; do
    echo "================================================================"
    echo "[verify_stages] stage ${stage}: optimise + compile"
    echo "================================================================"
    "${PYTHON}" -m "utils.stages.stage${stage}"

    bin="${EXP_DIR}/velocity_stage${stage}"
    if [[ ! -x "${bin}" ]]; then
        echo "[verify_stages] stage ${stage}: ${bin} did not build" >&2
        fail=$((fail + 1))
        continue
    fi

    out_dir="${GOTWANT_DIR}/stage${stage}"
    mkdir -p "${out_dir}"

    echo
    echo "[verify_stages] stage ${stage}: running on TS=${TIMESTEPS}"
    "${bin}" \
        --data "${DATA}" \
        --reps "${REPS}" \
        --timesteps "${TIMESTEPS}" \
        --output-dir "${out_dir}"

    echo
    echo "[verify_stages] stage ${stage}: comparing got vs want in ${out_dir}"
    if "${PYTHON}" utils/compare_got_and_want.py \
            -r "${out_dir}" \
            -t "${TIMESTEPS}"; then
        echo "[verify_stages] stage ${stage}: OK"
    else
        echo "[verify_stages] stage ${stage}: COMPARE FAILED" >&2
        fail=$((fail + 1))
    fi
    echo
done

echo "================================================================"
if (( fail == 0 )); then
    echo "[verify_stages] all stages passed."
    exit 0
else
    echo "[verify_stages] ${fail} stage(s) failed." >&2
    exit 1
fi
