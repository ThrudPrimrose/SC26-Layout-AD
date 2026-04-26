#!/usr/bin/env bash
# local_check.sh -- non-SLURM local driver for E7's nlev_first sweep.
#
# Builds + runs both nlev_first sm variants (sm0 / sm1) at timesteps
# 7 and 9, producing one output directory per (config, timestep) pair
# under ``results/local/<config>/ts<N>/``. Same on-disk layout as
# ``Figures/plot_paper_snapshot.sh`` expects.
#
# Reps default to 100 with 5 warm-ups (Hoefler & Belli convention),
# matching ``run_{daint,beverin}.sh``.
#
# Env knobs (override at the call site, e.g.
# ``REPS=10 bash tools/local_check.sh`` for a quick smoke run):
#
#   CONFIGS    space-separated list of stage5a configs (default: the
#              two nlev_first sm variants)
#   TIMESTEPS  comma-separated timesteps; each TS becomes a separate
#              binary invocation with its own output dir (default: 7,9)
#   REPS       timed reps per __program_ call             (default: 100)
#   WARMUP     untimed warm-up reps before --reps         (default: 5)
#   PYTHON     python interpreter                          (default: python)
#   DATA       data root                                   (default: ./data_r02b05)
#   RESULTS    output root                                 (default: ./results/local)

set -euo pipefail

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${EXP_DIR}"

PYTHON="${PYTHON:-python}"
CONFIGS="${CONFIGS:-nlev_first_lv0_sm0 nlev_first_lv0_sm1}"
TIMESTEPS="${TIMESTEPS:-7,9}"
REPS="${REPS:-100}"
WARMUP="${WARMUP:-5}"
DATA="${DATA:-${EXP_DIR}/data_r02b05}"
RESULTS="${RESULTS:-${EXP_DIR}/results/local}"

if [[ ! -d "${DATA}" ]] || [[ -z "$(ls -A "${DATA}" 2>/dev/null)" ]]; then
    echo "[local_check] data root empty: ${DATA}" >&2
    echo "[local_check] run tools/download_data.sh first." >&2
    exit 1
fi

# Same stage4 input symlink + JSON-regen guards as the SLURM scripts.
if [[ ! -d "${EXP_DIR}/codegen/stage4" ]] && [[ -d "${EXP_DIR}/SDFGs/stage4" ]]; then
    mkdir -p "${EXP_DIR}/codegen"
    ln -sfn "${EXP_DIR}/SDFGs/stage4" "${EXP_DIR}/codegen/stage4"
fi
LAYOUT_JSON="${EXP_DIR}/../E6_VelocityTendencies/access_analysis/layout_candidates.json"
if [[ ! -f "${LAYOUT_JSON}" ]]; then
    echo "[local_check] ${LAYOUT_JSON} missing; running select_loopnests.py"
    "${PYTHON}" "${EXP_DIR}/../E6_VelocityTendencies/access_analysis/select_loopnests.py"
fi

mkdir -p "${RESULTS}"

echo "[local_check] configs   = ${CONFIGS}"
echo "[local_check] timesteps = ${TIMESTEPS}"
echo "[local_check] reps      = ${REPS}  warmup = ${WARMUP}"
echo "[local_check] data      = ${DATA}"
echo "[local_check] results   = ${RESULTS}"
echo

fail=0
for CFG in ${CONFIGS}; do
    echo "================================================================"
    echo "[local_check] codegen + compile config=${CFG}"
    echo "================================================================"
    "${PYTHON}" -m utils.stages.stage5a --release --optimize --compile --config "${CFG}"

    bin="${EXP_DIR}/velocity_stage5a_${CFG}"
    if [[ ! -x "${bin}" ]]; then
        echo "[local_check] WARN: ${bin} not built; skipping ${CFG}" >&2
        fail=$((fail + 1))
        continue
    fi

    for TS in ${TIMESTEPS//,/ }; do
        out_dir="${RESULTS}/${CFG}/ts${TS}"
        mkdir -p "${out_dir}"
        echo
        echo "[local_check] running ${CFG} TS=${TS} reps=${REPS} warmup=${WARMUP}"
        "${bin}" \
            --data "${DATA}" \
            --reps "${REPS}" \
            --warmup "${WARMUP}" \
            --timesteps "${TS}" \
            --output-dir "${out_dir}"
    done
done

# Mirror any CSVs that landed inside the per-config codegen tree.
for d in "${EXP_DIR}/codegen/stage5a"/*/; do
    cfg="$(basename "${d}")"
    [[ "${CONFIGS}" == *"${cfg}"* ]] || continue
    mkdir -p "${RESULTS}/${cfg}"
    find "${d}" -maxdepth 2 -name '*.csv' -exec cp -v {} "${RESULTS}/${cfg}/" \; 2>/dev/null || true
done

echo
echo "================================================================"
if (( fail == 0 )); then
    echo "[local_check] all configs completed."
    echo "[local_check] outputs under ${RESULTS}/<config>/ts<N>/"
    exit 0
else
    echo "[local_check] ${fail} config(s) failed." >&2
    exit 1
fi
