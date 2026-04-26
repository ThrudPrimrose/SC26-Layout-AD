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
# Beverin (MI300A). See run_daint.sh for the full doc-comment.

# Note: no `set -e` / `pipefail`. A crashed binary on one config (e.g.
# std::out_of_range from a missing data field) must not abort the whole
# batch -- subsequent configs still need to run.
set -u

# --verify retains the per-timestep got/want numerical-comparison blobs
# (O(GB) each) for offline validation against a reference run. Default
# is to delete them after each binary invocation.
VERIFY=0
for arg in "$@"; do
  case "${arg}" in
    --verify) VERIFY=1 ;;
    *) echo "[E7 beverin] unrecognised arg: ${arg}" >&2 ;;
  esac
done

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"
cd "${EXP_DIR}"

[[ -d "${EXP_DIR}/data_r02b05" ]] && [[ -n "$(ls -A "${EXP_DIR}/data_r02b05" 2>/dev/null)" ]] \
    || bash tools/download_data.sh
export ICON_DATA_PATH="${ICON_DATA_PATH:-${EXP_DIR}/data_r02b05}"

# Defaults reproduce the paper's V1/V2/V6 winner-comparison (§IV-D)
# plus lv/sm ablations. Override per submission, e.g.
#   CONFIGS="winner_v1 winner_v2 winner_v6" sbatch run_beverin.sh
CONFIGS="${CONFIGS:-winner_v1_sm0 winner_v1_sm1 \
winner_v2_sm0 winner_v2_sm1 \
winner_v6_sm0 winner_v6_sm1 \
unpermuted_lv0_sm0 unpermuted_lv0_sm1 \
nlev_first_lv0_sm0 nlev_first_lv0_sm1 nlev_first_lv1_sm0 nlev_first_lv1_sm1 \
index_only_lv0_sm0 index_only_lv0_sm1}"

# Per-binary run knobs. Default to TS={7,9} -- the cells used in the
# paper's reported numbers; each timestep runs as a separate binary
# invocation (one CSV per ts/<config>). REPS=100 timed reps after
# WARMUP=5 untimed warm-up reps follows the Hoefler & Belli convention.
TIMESTEPS="${TIMESTEPS:-7,9}"
REPS="${REPS:-100}"
WARMUP="${WARMUP:-5}"

echo "[E7 beverin] host=$(hostname) threads=$OMP_NUM_THREADS data=$ICON_DATA_PATH"
echo "[E7 beverin] configs=$CONFIGS"
echo "[E7 beverin] timesteps=$TIMESTEPS reps=$REPS warmup=$WARMUP"

if [[ ! -d "${EXP_DIR}/codegen/stage4" ]] && [[ -d "${EXP_DIR}/SDFGs/stage4" ]]; then
  mkdir -p "${EXP_DIR}/codegen"
  ln -sfn "${EXP_DIR}/SDFGs/stage4" "${EXP_DIR}/codegen/stage4"
fi

# E7 reads E6's access-analysis output (layout_candidates.json). Prefer
# the committed copy; if it's missing -- e.g. cluster checkout pulled
# only E7 -- regenerate it from the committed analysis.md.
LAYOUT_JSON="${EXP_DIR}/../E6_VelocityTendencies/access_analysis/layout_candidates.json"
if [[ ! -f "${LAYOUT_JSON}" ]]; then
  echo "[E7 beverin] ${LAYOUT_JSON} missing; running select_loopnests.py"
  python "${EXP_DIR}/../E6_VelocityTendencies/access_analysis/select_loopnests.py"
fi

for CFG in ${CONFIGS}; do
  echo "[E7 beverin] codegen + compile config=${CFG}"
  python -m utils.stages.stage5a --release --optimize --compile --config "${CFG}"

  bin="${EXP_DIR}/velocity_stage5a_${CFG}"
  if [[ ! -x "${bin}" ]]; then
    echo "[E7 beverin] WARN: ${bin} not built; skipping run for ${CFG}" >&2
    continue
  fi

  # Run the binary once per timestep so each timestep lands in its
  # own output dir (results/beverin/<config>/ts<N>/...). Stdout/stderr
  # are tee'd to ${out_dir}/run.txt so the slurm log keeps the live
  # output AND the per-timestep TXT records the same stream
  # (icon-artifacts/velocity convention). When --verify is NOT passed
  # the got/want numerical-comparison blobs (O(GB) per pair) are
  # deleted after the run; --verify keeps them for one-shot reference
  # comparison.
  for TS in ${TIMESTEPS//,/ }; do
    out_dir="${EXP_DIR}/results/beverin/${CFG}/ts${TS}"
    mkdir -p "${out_dir}"
    txt="${out_dir}/run.txt"
    echo "[E7 beverin] running ${CFG} TS=${TS} reps=${REPS} warmup=${WARMUP} -> ${txt}"
    {
      echo "=== ${CFG} ts=${TS} reps=${REPS} warmup=${WARMUP} ==="
      "${bin}" \
          --data "${ICON_DATA_PATH}" \
          --reps "${REPS}" \
          --warmup "${WARMUP}" \
          --timesteps "${TS}" \
          --output-dir "${out_dir}"
    } 2>&1 | tee -a "${txt}"
    if [[ "${VERIFY}" -eq 0 ]]; then
      find "${out_dir}" -maxdepth 1 \( -name '*.got' -o -name '*.want' \) -delete
    fi
  done
done

# Mirror any CSVs that landed inside the per-config codegen tree.
for d in "${EXP_DIR}/codegen/stage5a"/*/; do
  cfg="$(basename "${d}")"
  mkdir -p "${EXP_DIR}/results/beverin/${cfg}"
  find "${d}" -maxdepth 2 -name '*.csv' -exec cp -v {} "${EXP_DIR}/results/beverin/${cfg}/" \;
done

echo "[E7 beverin] done. CSVs under results/beverin/<config>/"
