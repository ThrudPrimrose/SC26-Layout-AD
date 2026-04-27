#!/usr/bin/env bash
#SBATCH --job-name=E8_velocity_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=24:00:00
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=192
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=beverin_full_permutations_8/E8_velocity_beverin_%j.out
#SBATCH --error=beverin_full_permutations_8/E8_velocity_beverin_%j.err
#
# E8 / Full velocity tendencies (LEGACY pipeline) on Beverin (MI300A).
# See run_daint.sh for the full doc-comment.

set -u

# --dry-run: list configs that *would* run and exit. Skips the slow
# setup (data download, aenum install) so it's safe to invoke as
# ``bash run_beverin.sh --dry-run`` from a login node.
DRY_RUN=0
for arg in "$@"; do
  case "${arg}" in
    --dry-run) DRY_RUN=1 ;;
    *) echo "[E8 beverin] unrecognised arg: ${arg}" >&2 ;;
  esac
done

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

# E8 (legacy stage-8 pipeline) requires DaCe on f2dace/staging.
# Pin before sourcing activate.sh so its DACE_BRANCH-driven checkout
# switch goes the right way (default is yakup/dev, correct for E1-E7).
export DACE_BRANCH="f2dace/staging"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

# Stage 8 codegen pulls in dace.frontend.fortran, which on f2dace/staging
# imports ``aenum`` -- not in DaCe's setup.py extras, so a clean
# yakup-env may lack it. ``pip install`` is idempotent (no-op when the
# wheel is already in the venv); run quietly so it doesn't pollute
# the .out log. Skip in dry-run -- listing configs doesn't need it.
if (( DRY_RUN == 0 )); then
  python -m pip install --quiet aenum 2>/dev/null || true
fi

mkdir -p "${EXP_DIR}/beverin_full_permutations_8"
cd "${EXP_DIR}"

# ICON R02B05 dataset. See run_daint.sh for the full doc-comment.
# Order: existing E8 data > E7's data (symlink via LOCAL_DATA_DIR) > fresh download.
DOWNLOAD_DATA_SH="${EXP_DIR}/../E7_FullVelocityTendencies/tools/download_data.sh"
E7_DATA_DIR="${EXP_DIR}/../E7_FullVelocityTendencies/data_r02b05"
if (( DRY_RUN == 1 )); then
    : # Skip dataset fetch -- not needed for a config listing.
elif [[ -d "${EXP_DIR}/data_r02b05" ]] && [[ -n "$(ls -A "${EXP_DIR}/data_r02b05" 2>/dev/null)" ]]; then
    :
elif [[ -d "${E7_DATA_DIR}" ]] && [[ -n "$(ls -A "${E7_DATA_DIR}" 2>/dev/null)" ]]; then
    echo "[E8 beverin] data_r02b05 already in E7 tree; symlinking via LOCAL_DATA_DIR mode"
    LOCAL_DATA_DIR="${E7_DATA_DIR}" OUTPUT_DIR="${EXP_DIR}/data_r02b05" \
        bash "${DOWNLOAD_DATA_SH}"
else
    echo "[E8 beverin] no on-disk data found; fetching into ${EXP_DIR}/data_r02b05"
    OUTPUT_DIR="${EXP_DIR}/data_r02b05" bash "${DOWNLOAD_DATA_SH}"
fi
export ICON_DATA_PATH="${ICON_DATA_PATH:-${EXP_DIR}/data_r02b05}"

LAYOUT_JSON="${EXP_DIR}/../E6_VelocityTendencies/access_analysis/layout_candidates.json"
WINNERS_JSON="${EXP_DIR}/../E6_VelocityTendencies/access_analysis/winners.json"
if [[ ! -f "${LAYOUT_JSON}" ]]; then
    echo "[E8 beverin] ${LAYOUT_JSON} missing; running select_loopnests.py"
    python "${EXP_DIR}/../E6_VelocityTendencies/access_analysis/select_loopnests.py"
fi
if [[ ! -f "${WINNERS_JSON}" ]]; then
    echo "[E8 beverin] ${WINNERS_JSON} missing; running derive_winners.py"
    python "${EXP_DIR}/../E6_VelocityTendencies/access_analysis/derive_winners.py" || true
fi

# Default CONFIGS unset -> run_stage8_permutations.py picks the curated
# sweep (see run_daint.sh for the rationale). Override with a
# comma-separated CONFIGS env to pin a specific subset.
CONFIGS="${CONFIGS:-}"
REPS="${REPS:-100}"

echo "[E8 beverin] host=$(hostname) data=${ICON_DATA_PATH}"
echo "[E8 beverin] configs=${CONFIGS}  reps=${REPS}  dry_run=${DRY_RUN}"

# run_stage8_permutations.py needs BEVERIN=1 to pick the beverin
# OUT_DIR (beverin_full_permutations_8 vs daint_full_permutations_8).
DRY_FLAG=""
(( DRY_RUN == 1 )) && DRY_FLAG="--dry-run"
CFG_FLAG=""
[[ -n "${CONFIGS}" ]] && CFG_FLAG="--configs ${CONFIGS}"
BEVERIN=1 python run_stage8_permutations.py ${CFG_FLAG} --reps "${REPS}" ${DRY_FLAG}

if (( DRY_RUN == 1 )); then
  echo "[E8 beverin] dry-run complete -- no compile or execution."
else
  echo "[E8 beverin] done. TXTs under beverin_full_permutations_8/"
fi
