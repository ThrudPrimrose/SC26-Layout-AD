#!/usr/bin/env bash
#SBATCH --job-name=E8_velocity_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=18:00:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=288
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=daint_full_permutations_8/E8_velocity_daint_%j.out
#SBATCH --error=daint_full_permutations_8/E8_velocity_daint_%j.err
#
# E8 / Full velocity tendencies (LEGACY pipeline) on Daint.Alps.
#
# Drives sc26_layout/permute_stage8.py via run_stage8_permutations.py.
# Mirrors E7's --config submission interface but uses the legacy
# stage-8 codegen pipeline (no yakup/dev DaCe; OffloadVelocityToGPU
# replaced by the icon-artifacts/sc26 single-shot transform). Output
# files:
#   daint_full_permutations_8/<config>_<shuffled|unshuffled>.txt
#
# CONFIGS env override (default: V1 / V2 / V6 winners):
#   CONFIGS="winner_v1,winner_v6" sbatch run_daint.sh

# Note: no `set -e` / `pipefail`. A single config crashing must not
# abort the rest of the batch (run_stage8_permutations.py already
# logs and continues per-config).
set -u

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/daint_full_permutations_8"
cd "${EXP_DIR}"

# ICON R02B05 dataset. Three cases, in order of preference:
#   1. ${EXP_DIR}/data_r02b05 already populated -> use as-is (no-op).
#   2. ../E7_FullVelocityTendencies/data_r02b05 already populated ->
#      symlink to it via download_data.sh's LOCAL_DATA_DIR mode (no
#      download, no copy; both experiments share the on-disk data).
#   3. Otherwise -> download into ${EXP_DIR}/data_r02b05 directly so
#      E8 is self-contained on first submission.
# All cases are routed through E7's tools/download_data.sh, which
# handles idempotency, sha256 verification, and stale-spack-xz fallback.
DOWNLOAD_DATA_SH="${EXP_DIR}/../E7_FullVelocityTendencies/tools/download_data.sh"
E7_DATA_DIR="${EXP_DIR}/../E7_FullVelocityTendencies/data_r02b05"
if [[ -d "${EXP_DIR}/data_r02b05" ]] && [[ -n "$(ls -A "${EXP_DIR}/data_r02b05" 2>/dev/null)" ]]; then
    : # Already populated -- skip.
elif [[ -d "${E7_DATA_DIR}" ]] && [[ -n "$(ls -A "${E7_DATA_DIR}" 2>/dev/null)" ]]; then
    echo "[E8 daint] data_r02b05 already in E7 tree; symlinking via LOCAL_DATA_DIR mode"
    LOCAL_DATA_DIR="${E7_DATA_DIR}" OUTPUT_DIR="${EXP_DIR}/data_r02b05" \
        bash "${DOWNLOAD_DATA_SH}"
else
    echo "[E8 daint] no on-disk data found; fetching into ${EXP_DIR}/data_r02b05"
    OUTPUT_DIR="${EXP_DIR}/data_r02b05" bash "${DOWNLOAD_DATA_SH}"
fi
export ICON_DATA_PATH="${ICON_DATA_PATH:-${EXP_DIR}/data_r02b05}"

# E6 layout_candidates.json + winners.json (regenerate if missing).
LAYOUT_JSON="${EXP_DIR}/../E6_VelocityTendencies/access_analysis/layout_candidates.json"
WINNERS_JSON="${EXP_DIR}/../E6_VelocityTendencies/access_analysis/winners.json"
if [[ ! -f "${LAYOUT_JSON}" ]]; then
    echo "[E8 daint] ${LAYOUT_JSON} missing; running select_loopnests.py"
    python "${EXP_DIR}/../E6_VelocityTendencies/access_analysis/select_loopnests.py"
fi
if [[ ! -f "${WINNERS_JSON}" ]]; then
    echo "[E8 daint] ${WINNERS_JSON} missing; running derive_winners.py"
    python "${EXP_DIR}/../E6_VelocityTendencies/access_analysis/derive_winners.py" || true
fi

# Default CONFIGS reproduces the V1 / V2 / V6 winner comparison from
# the AD; override via CONFIGS env (comma-separated for
# run_stage8_permutations.py's --configs).
CONFIGS="${CONFIGS:-winner_v1,winner_v2,winner_v6}"
REPS="${REPS:-100}"

echo "[E8 daint] host=$(hostname) data=${ICON_DATA_PATH}"
echo "[E8 daint] configs=${CONFIGS}  reps=${REPS}"

# run_stage8_permutations.py compiles each config and runs both
# shuffled (_ms = map-shuffled) and unshuffled (_mu = map-unshuffled)
# variants, capturing stdout to ${OUT_DIR}/<config>_<shuffled|unshuffled>.txt.
# It logs but doesn't abort on a per-config failure -- by design, the
# whole batch keeps going.
python run_stage8_permutations.py --configs "${CONFIGS}" --reps "${REPS}"

echo "[E8 daint] done. TXTs under daint_full_permutations_8/"
