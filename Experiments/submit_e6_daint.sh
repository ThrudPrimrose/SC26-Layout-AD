#!/usr/bin/env bash
# Submit run_daint.sh for every E6 subtask (loopnest_1..6 + full_velocity_tendencies).
# Each sbatch is invoked from the subtask folder so SLURM_SUBMIT_DIR lands
# on that folder and the scripts' path resolution is correct.
#
# Usage:  bash submit_e6_daint.sh [L1 L2 ...]
#   With no args, submits the six loopnests (L1..L6). FVT is intentionally
#   excluded from the default because it is the longest job (18 h wallclock).
#   To include it, pass it explicitly:
#       bash submit_e6_daint.sh L1 L2 L3 L4 L5 L6 FVT
#       bash submit_e6_daint.sh FVT          # just the full module
#
# Env overrides:
#   DRY_RUN=1  print the commands instead of running them.
#
# Notes:
#   - L1..L6 are pure C++/CUDA microbenchmarks; they don't import dace and
#     don't trigger any branch switch (activate.sh's DaCe-branch logic is
#     opt-in on DACE_BRANCH being explicitly exported). E7 pins yakup/dev;
#     E8 pins f2dace/staging. No env needed here.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
E6_ROOT="${SCRIPT_DIR}/E6_VelocityTendencies"

declare -A EXP_DIR=(
    [L1]="loopnest_1"
    [L2]="loopnest_2"
    [L3]="loopnest_3"
    [L4]="loopnest_4"
    [L5]="loopnest_5"
    [L6]="loopnest_6"
    [FVT]="full_velocity_tendencies"
)
DEFAULT_ORDER=(L1 L2 L3 L4 L5 L6)   # FVT excluded by default (18 h job)

if (( $# == 0 )); then
    targets=("${DEFAULT_ORDER[@]}")
else
    targets=("$@")
fi

jobs_log="${SCRIPT_DIR}/.submit_e6_daint_jobs.log"
: >"${jobs_log}"

for tag in "${targets[@]}"; do
    dir="${EXP_DIR[$tag]:-}"
    if [[ -z "${dir}" ]]; then
        echo "[skip] unknown tag: ${tag}" >&2
        continue
    fi
    script="${E6_ROOT}/${dir}/run_daint.sh"
    if [[ ! -f "${script}" ]]; then
        echo "[skip] missing: ${script}" >&2
        continue
    fi

    if [[ "${DRY_RUN:-0}" == "1" ]]; then
        echo "[dry-run] (cd ${E6_ROOT}/${dir} && sbatch run_daint.sh)"
        continue
    fi

    pushd "${E6_ROOT}/${dir}" >/dev/null
    out=$(sbatch run_daint.sh)
    popd >/dev/null

    jobid=$(awk '{print $NF}' <<<"${out}")
    printf '%-30s %-8s %s\n' "E6/${dir}" "${jobid}" "${out}" | tee -a "${jobs_log}"
done

if [[ "${DRY_RUN:-0}" != "1" ]]; then
    echo
    echo "[submit_e6_daint] queue snapshot:"
    squeue --me --format='%.10i %.22j %.8T %.10M %.9l %R' 2>/dev/null || echo "  (squeue unavailable)"
    echo
    echo "Log: ${jobs_log}"
fi
