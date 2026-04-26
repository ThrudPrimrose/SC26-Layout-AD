#!/usr/bin/env bash
#SBATCH --job-name=E7_velocity_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=06:00:00
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
# Daint.Alps. Runs stage 6 (the permutation driver) against the stage 5
# SDFGs that ship with the AD. Always uses DaCe yakup/dev (no branch
# switching). For an F90 -> stage 5 regeneration, run
# tools/regenerate_baselines.sh first.

set -euo pipefail

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"
cd "${EXP_DIR}"

# R02B05 / nproma=20480 dataset. tools/download_data.sh fetches into
# ${EXP_DIR}/data_r02b05/; idempotent (skips if non-empty).
[[ -d "${EXP_DIR}/data_r02b05" ]] && [[ -n "$(ls -A "${EXP_DIR}/data_r02b05" 2>/dev/null)" ]] \
    || bash tools/download_data.sh
export ICON_DATA_PATH="${ICON_DATA_PATH:-${EXP_DIR}/data_r02b05}"

# Layout configs to run. Default: the three named heuristics emitted by
# utils.passes.permute_configs.configs_from_candidates from the E6
# access analysis. Override at submission time, e.g.
#   CONFIGS="unpermuted curated_nlev_first" sbatch run_daint.sh
CONFIGS="${CONFIGS:-unpermuted nlev_first index_only}"

echo "[E7 daint] host=$(hostname) threads=$OMP_NUM_THREADS data=$ICON_DATA_PATH"
echo "[E7 daint] configs=$CONFIGS"

# Stage 6 reads codegen/stage5/<variant>.sdfgz. The shipped stage 5 SDFGs
# live next to this script under SDFGs/stage5/; symlink them in place if
# tools/regenerate_baselines.sh has not been run.
if [[ ! -d "${EXP_DIR}/codegen/stage5" ]] && [[ -d "${EXP_DIR}/SDFGs/stage5" ]]; then
  mkdir -p "${EXP_DIR}/codegen"
  ln -sfn "${EXP_DIR}/SDFGs/stage5" "${EXP_DIR}/codegen/stage5"
fi

for CFG in ${CONFIGS}; do
  echo "[E7 daint] running config=${CFG}"
  python -m utils.stages.stage6 --optimize --compile --config "${CFG}"
done

# stage6.py writes CSVs into codegen/stage6/<config>/; mirror them into
# the AD's per-platform results tree so plot_results.sh picks them up.
for d in "${EXP_DIR}/codegen/stage6"/*/; do
  cfg="$(basename "${d}")"
  mkdir -p "${EXP_DIR}/results/daint/${cfg}"
  find "${d}" -maxdepth 2 -name '*.csv' -exec cp -v {} "${EXP_DIR}/results/daint/${cfg}/" \;
done

echo "[E7 daint] done. CSVs under results/daint/<config>/"
