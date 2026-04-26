#!/usr/bin/env bash
#SBATCH --job-name=E7_velocity_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=06:00:00
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

set -euo pipefail

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"
cd "${EXP_DIR}"

[[ -d "${EXP_DIR}/data_r02b05" ]] && [[ -n "$(ls -A "${EXP_DIR}/data_r02b05" 2>/dev/null)" ]] \
    || bash tools/download_data.sh
export ICON_DATA_PATH="${ICON_DATA_PATH:-${EXP_DIR}/data_r02b05}"

CONFIGS="${CONFIGS:-unpermuted nlev_first index_only}"

echo "[E7 beverin] host=$(hostname) threads=$OMP_NUM_THREADS data=$ICON_DATA_PATH"
echo "[E7 beverin] configs=$CONFIGS"

if [[ ! -d "${EXP_DIR}/codegen/stage4" ]] && [[ -d "${EXP_DIR}/SDFGs/stage4" ]]; then
  mkdir -p "${EXP_DIR}/codegen"
  ln -sfn "${EXP_DIR}/SDFGs/stage4" "${EXP_DIR}/codegen/stage4"
fi

for CFG in ${CONFIGS}; do
  echo "[E7 beverin] running config=${CFG}"
  python -m utils.stages.stage5a --release --optimize --compile --config "${CFG}"
done

for d in "${EXP_DIR}/codegen/stage5a"/*/; do
  cfg="$(basename "${d}")"
  mkdir -p "${EXP_DIR}/results/beverin/${cfg}"
  find "${d}" -maxdepth 2 -name '*.csv' -exec cp -v {} "${EXP_DIR}/results/beverin/${cfg}/" \;
done

echo "[E7 beverin] done. CSVs under results/beverin/<config>/"
