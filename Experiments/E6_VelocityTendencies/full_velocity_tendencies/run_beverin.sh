#!/usr/bin/env bash
#SBATCH --job-name=E6FVT_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=18:00:00
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=192
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/beverin/E6FVT_beverin_%j.out
#SBATCH --error=results/beverin/E6FVT_beverin_%j.err
#
# E6 / full_velocity_tendencies -- end-to-end SDFG permutation sweep on
# Beverin (MI300A). See run_daint.sh for documentation.

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../../common" && pwd)"

export DACE_BRANCH="${DACE_BRANCH:-f2dace/staging}"
source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"

export _RELEASE=1
export GENCODE_NUMBER=90
export RM_TIMERS=1
export _TBLOCK_DIMS="32,4,1"
export _REDUCE_BITWIDTH_TRANSFORMATION=0
export _SUFFIX="full_"
export V2=0

export PYTHONPATH="${EXP_DIR}/scripts:${EXP_DIR}:${PYTHONPATH:-}"

REPS="${REPS:-100}"
CONFIGS_LIST="${CONFIGS:-nlev_first index_only unpermuted}"
STAGE_SET="${STAGE_SET:-4 8}"

echo "[E6FVT beverin] host=$(hostname) reps=$REPS stages='$STAGE_SET' configs='$CONFIGS_LIST'"
echo "[E6FVT beverin] DACE_BRANCH=$DACE_BRANCH PYTHONPATH=$PYTHONPATH"

for STAGE in ${STAGE_SET}; do
  export _STAGE=${STAGE}
  STAGE_OUT="${EXP_DIR}/results/beverin/stage${STAGE}"
  mkdir -p "${STAGE_OUT}"
  cd "${STAGE_OUT}"

  RUNNER="${EXP_DIR}/scripts/run_stage${STAGE}_permutations.py"
  if [[ ! -f "${RUNNER}" ]]; then
    echo "[E6FVT beverin] skip stage${STAGE} -- runner ${RUNNER} not found"
    continue
  fi

  for CFG in ${CONFIGS_LIST}; do
    case "$CFG" in
      unpermuted)
        python "${RUNNER}" --unpermuted --reps "${REPS}"
        ;;
      full_sweep)
        python "${RUNNER}" --reps "${REPS}"
        ;;
      *)
        python "${RUNNER}" --configs="${CFG}" --reps "${REPS}"
        ;;
    esac
  done
done

echo "[E6FVT beverin] done. Outputs under results/beverin/stage{4,8}/"
