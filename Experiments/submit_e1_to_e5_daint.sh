#!/usr/bin/env bash
# Submit run_daint.sh for E1_MatrixAdd, E2_Conjugation, ..., E5_USXX.
# Each sbatch is invoked from the experiment folder so SLURM_SUBMIT_DIR
# lands on that folder and the scripts' path resolution is correct.
#
# Usage:  bash submit_e1_to_e5_daint.sh [E1 E2 ...]
#   With no args, submits all five (E1..E5).
#   With args, submits only the named experiments (order preserved).
#
# For E0 (NUMA baseline), run `sbatch E0_NUMA/run_daint.sh` directly.
# For E6, use submit_e6_daint.sh.
#
# Env overrides:
#   DRY_RUN=1  print the commands instead of running them.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

declare -A EXP_DIR=(
    [E1]="E1_MatrixAdd"
    [E2]="E2_Conjugation"
    [E3]="E3_Transpose"
    [E4]="E4_GAS"
    [E5]="E5_USXX"
)
DEFAULT_ORDER=(E1 E2 E3 E4 E5)

if (( $# == 0 )); then
    targets=("${DEFAULT_ORDER[@]}")
else
    targets=("$@")
fi

jobs_log="${SCRIPT_DIR}/.submit_e1_to_e5_daint_jobs.log"
: >"${jobs_log}"

for tag in "${targets[@]}"; do
    dir="${EXP_DIR[$tag]:-}"
    if [[ -z "${dir}" ]]; then
        echo "[skip] unknown tag: ${tag}" >&2
        continue
    fi
    script="${SCRIPT_DIR}/${dir}/run_daint.sh"
    if [[ ! -f "${script}" ]]; then
        echo "[skip] missing: ${script}" >&2
        continue
    fi

    if [[ "${DRY_RUN:-0}" == "1" ]]; then
        echo "[dry-run] (cd ${SCRIPT_DIR}/${dir} && sbatch run_daint.sh)"
        continue
    fi

    pushd "${SCRIPT_DIR}/${dir}" >/dev/null
    out=$(sbatch run_daint.sh)
    popd >/dev/null

    jobid=$(awk '{print $NF}' <<<"${out}")
    printf '%-25s %-8s %s\n' "${dir}" "${jobid}" "${out}" | tee -a "${jobs_log}"
done

if [[ "${DRY_RUN:-0}" != "1" ]]; then
    echo
    echo "[submit_e1_to_e5_daint] queue snapshot:"
    squeue --me --format='%.10i %.20j %.8T %.10M %.9l %R' 2>/dev/null || echo "  (squeue unavailable)"
    echo
    echo "Log: ${jobs_log}"
fi
