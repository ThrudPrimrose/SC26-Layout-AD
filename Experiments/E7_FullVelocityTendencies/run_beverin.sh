#!/usr/bin/env bash
#SBATCH --job-name=E7_velocity_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=18:00:00
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=192
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/beverin/E7_velocity_beverin_%j.out
#SBATCH --error=results/beverin/E7_velocity_beverin_%j.err
#
# E7 / Full velocity tendencies -- GPU layout-permutation sweep on
# Beverin (MI300A). Mirrors run_daint.sh; see that file's header for
# the layered-sweep description (named ablations + winners cross-product).

set -u

VERIFY=0
for arg in "$@"; do
  case "${arg}" in
    --verify) VERIFY=1 ;;
    *) echo "[E7 beverin] unrecognised arg: ${arg}" >&2 ;;
  esac
done

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

# E7 (WIP SDFG-driven pipeline) requires DaCe on yakup/dev. Pin
# explicitly: activate.sh's DaCe checkout is opt-in on DACE_BRANCH.
# DO NOT run E7 and E8 concurrently against the same DaCe clone --
# E8 pins f2dace/staging, E7 pins yakup/dev, the two branches do not
# share submodule SHAs and will fight over the working tree.
export DACE_BRANCH="yakup/dev"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"
cd "${EXP_DIR}"

[[ -d "${EXP_DIR}/data_r02b05" ]] && [[ -n "$(ls -A "${EXP_DIR}/data_r02b05" 2>/dev/null)" ]] \
    || bash tools/download_data.sh
export ICON_DATA_PATH="${ICON_DATA_PATH:-${EXP_DIR}/data_r02b05}"

NAMED_CONFIGS="${NAMED_CONFIGS:-unpermuted_lv0_sm0 unpermuted_lv0_sm1 \
nlev_first_lv0_sm0 nlev_first_lv0_sm1 nlev_first_lv1_sm0 nlev_first_lv1_sm1 \
index_only_lv0_sm0 index_only_lv0_sm1}"

TIMESTEPS="${TIMESTEPS:-7,9}"
REPS="${REPS:-100}"
WARMUP="${WARMUP:-5}"

E6_DIR="${EXP_DIR}/../E6_VelocityTendencies"
WINNERS_JSON="${WINNERS_JSON:-${E6_DIR}/full_velocity_tendencies/layout_crossproduct_winners.json}"
WINNERS_NLEV="${WINNERS_NLEV:-128}"
REGEN_WINNERS="${REGEN_WINNERS:-0}"

echo "[E7 beverin] host=$(hostname) threads=$OMP_NUM_THREADS data=$ICON_DATA_PATH"
echo "[E7 beverin] named_configs=$NAMED_CONFIGS"
echo "[E7 beverin] timesteps=$TIMESTEPS reps=$REPS warmup=$WARMUP"
echo "[E7 beverin] winners_json=$WINNERS_JSON (nlev=$WINNERS_NLEV)"

if [[ ! -d "${EXP_DIR}/codegen/stage4" ]] && [[ -d "${EXP_DIR}/SDFGs/stage4" ]]; then
  mkdir -p "${EXP_DIR}/codegen"
  ln -sfn "${EXP_DIR}/SDFGs/stage4" "${EXP_DIR}/codegen/stage4"
fi

LAYOUT_JSON="${E6_DIR}/access_analysis/layout_candidates.json"
if [[ ! -f "${LAYOUT_JSON}" ]]; then
  echo "[E7 beverin] ${LAYOUT_JSON} missing; running select_loopnests.py"
  python "${E6_DIR}/access_analysis/select_loopnests.py"
fi

if [[ "${REGEN_WINNERS}" -eq 1 ]] || [[ ! -f "${WINNERS_JSON}" ]]; then
  echo "[E7 beverin] (re)generating winners JSON at nlev=${WINNERS_NLEV}"
  ( cd "${E6_DIR}" && python3 generate_winners.py --nlev "${WINNERS_NLEV}" )
fi

# ── Helpers ──
have_all_outputs() {
  local cfg="$1" ts
  for ts in ${TIMESTEPS//,/ }; do
    [[ -f "${EXP_DIR}/results/beverin/${cfg}/ts${ts}/run.txt" ]] || return 1
  done
  return 0
}

run_binary_per_ts() {
  local CFG="$1" bin="$2" sweep_label="$3"
  if [[ ! -x "${bin}" ]]; then
    echo "[E7 beverin] WARN: ${bin} not built; skipping ${sweep_label} ${CFG}" >&2
    return
  fi
  for TS in ${TIMESTEPS//,/ }; do
    local out_dir="${EXP_DIR}/results/beverin/${CFG}/ts${TS}"
    mkdir -p "${out_dir}"
    local txt="${out_dir}/run.txt"
    echo "[E7 beverin] running ${sweep_label} ${CFG} TS=${TS} reps=${REPS} warmup=${WARMUP} -> ${txt}"
    {
      echo "=== ${CFG} ts=${TS} reps=${REPS} warmup=${WARMUP} ==="
      "${bin}" \
          --data "${ICON_DATA_PATH}" \
          --reps "${REPS}" \
          --warmup "${WARMUP}" \
          --timesteps "${TS}" \
          --output-dir "${out_dir}"
    } 2>&1 | tee -a "${txt}"
    local rc=${PIPESTATUS[0]:-0}
    if (( rc != 0 )); then
      echo "[E7 beverin] WARN: ${CFG} ts=${TS} aborted (rc=${rc}); continuing" >&2
      echo "=== ABORTED rc=${rc} ===" >> "${txt}"
    fi
    if [[ "${VERIFY}" -eq 0 ]]; then
      find "${out_dir}" -maxdepth 1 \( -name '*.got' -o -name '*.want' \) -delete
      find "${out_dir}" -maxdepth 1 -name 'core*' -delete
    fi
  done
}

# Enumerate winners cnames once.
echo "[E7 beverin] enumerating WINNERS cross-product"
mapfile -t V123_CFGS < <(
  python tools/run_layout_configs.py --dry-run --v123-json "${WINNERS_JSON}" 2>/dev/null \
    | awk -F'[][]' '/^  \[v123_/{print $2}'
)
echo "[E7 beverin] WINNERS unique configs: ${#V123_CFGS[@]}"

# ── Per-config sweep: build → run → next config (resumable) ──
for CFG in ${NAMED_CONFIGS}; do
  if have_all_outputs "${CFG}"; then
    echo "[E7 beverin] skip NAMED ${CFG}: results/beverin/${CFG}/ts{${TIMESTEPS}}/run.txt all present"
    continue
  fi
  echo "[E7 beverin] codegen + compile NAMED ${CFG}"
  python -m utils.stages.stage5a --release --optimize --compile --config "${CFG}"
  run_binary_per_ts "${CFG}" "${EXP_DIR}/velocity_stage5a_${CFG}" "NAMED"
done

for CFG in "${V123_CFGS[@]}"; do
  if have_all_outputs "${CFG}"; then
    echo "[E7 beverin] skip WINNERS ${CFG}: results/beverin/${CFG}/ts{${TIMESTEPS}}/run.txt all present"
    continue
  fi
  echo "[E7 beverin] codegen + compile WINNERS ${CFG}"
  python tools/run_layout_configs.py --optimize --compile \
      --v123-json "${WINNERS_JSON}" --config "${CFG}"
  run_binary_per_ts "${CFG}" "${EXP_DIR}/velocity_stage6_${CFG}" "WINNERS"
done

for d in "${EXP_DIR}/codegen/stage5a"/*/ "${EXP_DIR}/codegen/stage6"/v123_*/; do
  [[ -d "${d}" ]] || continue
  cfg="$(basename "${d}")"
  mkdir -p "${EXP_DIR}/results/beverin/${cfg}"
  find "${d}" -maxdepth 2 -name '*.csv' -exec cp -v {} "${EXP_DIR}/results/beverin/${cfg}/" \;
done

echo "[E7 beverin] done. CSVs under results/beverin/<config>/"
