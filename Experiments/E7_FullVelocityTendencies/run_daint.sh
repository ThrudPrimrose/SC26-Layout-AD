#!/usr/bin/env bash
#SBATCH --job-name=E7_velocity_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=12:00:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=288
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/daint/E7_velocity_daint_%j.out
#SBATCH --error=results/daint/E7_velocity_daint_%j.err
#
# E7 / Full velocity tendencies -- GPU layout-permutation sweep on
# Daint.Alps. Runs two layered sweeps:
#
#   1. Always-on named ablations: ``unpermuted_lv*_sm*``,
#      ``nlev_first_lv*_sm*``, ``index_only_lv*_sm*`` -- compiled via
#      ``utils.stages.stage5a`` to ``velocity_stage5a_<CFG>`` binaries.
#   2. Empirical winners cross-product: per-group V-id sets from
#      ``E6/generate_winners.py`` (default --nlev 256), expanded by
#      ``tools/run_layout_configs.py`` into ``velocity_stage6_v123_*``
#      binaries -- 64 unique permute-map signatures out of the
#      3**6 = 729 raw cells.
#
# Each binary is run once per timestep listed in ${TIMESTEPS}.

set -u

VERIFY=0
for arg in "$@"; do
  case "${arg}" in
    --verify) VERIFY=1 ;;
    *) echo "[E7 daint] unrecognised arg: ${arg}" >&2 ;;
  esac
done

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"
cd "${EXP_DIR}"

# R02B05 / nproma=20480 dataset.
[[ -d "${EXP_DIR}/data_r02b05" ]] && [[ -n "$(ls -A "${EXP_DIR}/data_r02b05" 2>/dev/null)" ]] \
    || bash tools/download_data.sh
export ICON_DATA_PATH="${ICON_DATA_PATH:-${EXP_DIR}/data_r02b05}"

# Always-on named ablations (independent of measured winners).
NAMED_CONFIGS="${NAMED_CONFIGS:-unpermuted_lv0_sm0 unpermuted_lv0_sm1 \
nlev_first_lv0_sm0 nlev_first_lv0_sm1 nlev_first_lv1_sm0 nlev_first_lv1_sm1 \
index_only_lv0_sm0 index_only_lv0_sm1}"

# Per-binary run knobs.
TIMESTEPS="${TIMESTEPS:-7,9}"
REPS="${REPS:-100}"
WARMUP="${WARMUP:-5}"

# Winners cross-product source. Regenerated from E6's loopnest CSVs
# whenever absent or REGEN_WINNERS=1. WINNERS_NLEV picks the slice
# the per-group winners are read from (default 256).
E6_DIR="${EXP_DIR}/../E6_VelocityTendencies"
WINNERS_JSON="${WINNERS_JSON:-${E6_DIR}/full_velocity_tendencies/layout_crossproduct_winners.json}"
WINNERS_NLEV="${WINNERS_NLEV:-256}"
REGEN_WINNERS="${REGEN_WINNERS:-0}"

echo "[E7 daint] host=$(hostname) threads=$OMP_NUM_THREADS data=$ICON_DATA_PATH"
echo "[E7 daint] named_configs=$NAMED_CONFIGS"
echo "[E7 daint] timesteps=$TIMESTEPS reps=$REPS warmup=$WARMUP"
echo "[E7 daint] winners_json=$WINNERS_JSON (nlev=$WINNERS_NLEV)"

# Stage 5a / runner read codegen/stage4/<variant>.sdfgz. Symlink the
# shipped frozen SDFGs in if the generated tree is empty.
if [[ ! -d "${EXP_DIR}/codegen/stage4" ]] && [[ -d "${EXP_DIR}/SDFGs/stage4" ]]; then
  mkdir -p "${EXP_DIR}/codegen"
  ln -sfn "${EXP_DIR}/SDFGs/stage4" "${EXP_DIR}/codegen/stage4"
fi

# E6 access-analysis JSON (consumed by both the named ablations and
# the winners cross-product runner).
LAYOUT_JSON="${E6_DIR}/access_analysis/layout_candidates.json"
if [[ ! -f "${LAYOUT_JSON}" ]]; then
  echo "[E7 daint] ${LAYOUT_JSON} missing; running select_loopnests.py"
  python "${E6_DIR}/access_analysis/select_loopnests.py"
fi

# Refresh the winners JSON from the latest loopnest CSVs (or generate
# it for the first time). The generator reads
# loopnest_{1..6}/results/{beverin,daint}/<kernel>_gpu.csv at the
# requested nlev slice and applies the >50% promotion rule.
if [[ "${REGEN_WINNERS}" -eq 1 ]] || [[ ! -f "${WINNERS_JSON}" ]]; then
  echo "[E7 daint] (re)generating winners JSON at nlev=${WINNERS_NLEV}"
  ( cd "${E6_DIR}" && python3 generate_winners.py --nlev "${WINNERS_NLEV}" )
fi

# ── Step 1: build named ablations via stage5a ──
for CFG in ${NAMED_CONFIGS}; do
  echo "[E7 daint] codegen + compile NAMED config=${CFG}"
  python -m utils.stages.stage5a --release --optimize --compile --config "${CFG}"
done

# ── Step 2: build winners cross-product via run_layout_configs.py ──
echo "[E7 daint] codegen + compile WINNERS cross-product"
python tools/run_layout_configs.py --optimize --compile \
    --v123-json "${WINNERS_JSON}"

# ── Step 3: iterate every produced binary, one per timestep ──
run_binary_per_ts() {
  local CFG="$1" bin="$2" sweep_label="$3"
  if [[ ! -x "${bin}" ]]; then
    echo "[E7 daint] WARN: ${bin} not built; skipping ${sweep_label} ${CFG}" >&2
    return
  fi
  for TS in ${TIMESTEPS//,/ }; do
    local out_dir="${EXP_DIR}/results/daint/${CFG}/ts${TS}"
    mkdir -p "${out_dir}"
    local txt="${out_dir}/run.txt"
    echo "[E7 daint] running ${sweep_label} ${CFG} TS=${TS} reps=${REPS} warmup=${WARMUP} -> ${txt}"
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
      echo "[E7 daint] WARN: ${CFG} ts=${TS} aborted (rc=${rc}); continuing" >&2
      echo "=== ABORTED rc=${rc} ===" >> "${txt}"
    fi
    if [[ "${VERIFY}" -eq 0 ]]; then
      find "${out_dir}" -maxdepth 1 \( -name '*.got' -o -name '*.want' \) -delete
      find "${out_dir}" -maxdepth 1 -name 'core*' -delete
    fi
  done
}

# Named ablation binaries.
for CFG in ${NAMED_CONFIGS}; do
  run_binary_per_ts "${CFG}" "${EXP_DIR}/velocity_stage5a_${CFG}" "NAMED"
done

# Winners cross-product binaries: one per ``codegen/stage6/v123_*/`` dir.
shopt -s nullglob
for d in "${EXP_DIR}/codegen/stage6"/v123_*/; do
  CFG="$(basename "${d}")"
  run_binary_per_ts "${CFG}" "${EXP_DIR}/velocity_stage6_${CFG}" "WINNERS"
done
shopt -u nullglob

# Mirror any CSVs that landed inside the per-config codegen trees.
for d in "${EXP_DIR}/codegen/stage5a"/*/ "${EXP_DIR}/codegen/stage6"/v123_*/; do
  [[ -d "${d}" ]] || continue
  cfg="$(basename "${d}")"
  mkdir -p "${EXP_DIR}/results/daint/${cfg}"
  find "${d}" -maxdepth 2 -name '*.csv' -exec cp -v {} "${EXP_DIR}/results/daint/${cfg}/" \;
done

echo "[E7 daint] done. CSVs under results/daint/<config>/"
