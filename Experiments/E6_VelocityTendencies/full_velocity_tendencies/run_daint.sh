#!/usr/bin/env bash
#SBATCH --job-name=E6FVT_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=18:00:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=288
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/daint/E6FVT_daint_%j.out
#SBATCH --error=results/daint/E6FVT_daint_%j.err
#
# E6 / full_velocity_tendencies -- end-to-end SDFG permutation sweep on
# Daint.Alps. Runs the layout-permutation driver for stage4 (CPU) and
# stage8 (GPU). DaCe must be on branch f2dace/staging with layout
# transformations applied; see sc26_layout/prepare.sh for the
# cherry-pick recipe.
#
# The permutation runner (scripts/run_stageN_permutations.py) compiles
# executables and writes timing files into `<host>_full_permutations_<N>/`
# under its working directory. We run from results/<host>/stage<N>/ so
# outputs land there directly.
#
# Env overrides:
#   REPS      default 100
#   CONFIGS   space list (default: "nlev_first index_only unpermuted")
#             use CONFIGS="full_sweep" for the full 95-config sweep
#   STAGE_SET default "4 8"  (space-separated stages to run)
#   DACE_BRANCH default f2dace/staging

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../../common" && pwd)"

export DACE_BRANCH="${DACE_BRANCH:-f2dace/staging}"
source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"

# Layout-transformation knobs picked up by scripts/run_stage*.py
export _RELEASE=1
export GENCODE_NUMBER=90a
export RM_TIMERS=1
export _TBLOCK_DIMS="32,16,1"
export _REDUCE_BITWIDTH_TRANSFORMATION=0
export _SUFFIX="full_"
export V2=0

# Expose scripts/ so `import utils.stages...` resolves, and sc26_layout/
# (it is symlinked inside scripts/, but we also add the parent for
# explicit clarity).
export PYTHONPATH="${EXP_DIR}/scripts:${EXP_DIR}:${PYTHONPATH:-}"

REPS="${REPS:-100}"
CONFIGS_LIST="${CONFIGS:-nlev_first index_only unpermuted}"
STAGE_SET="${STAGE_SET:-4 8}"

echo "[E6FVT daint] host=$(hostname) reps=$REPS stages='$STAGE_SET' configs='$CONFIGS_LIST'"
echo "[E6FVT daint] DACE_BRANCH=$DACE_BRANCH PYTHONPATH=$PYTHONPATH"

for STAGE in ${STAGE_SET}; do
  export _STAGE=${STAGE}
  STAGE_OUT="${EXP_DIR}/results/daint/stage${STAGE}"
  mkdir -p "${STAGE_OUT}"
  cd "${STAGE_OUT}"

  RUNNER="${EXP_DIR}/scripts/run_stage${STAGE}_permutations.py"
  if [[ ! -f "${RUNNER}" ]]; then
    echo "[E6FVT daint] skip stage${STAGE} -- runner ${RUNNER} not found"
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

echo "[E6FVT daint] done. Outputs under results/daint/stage{4,8}/"
