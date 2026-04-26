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
# Daint.Alps. Runs stage 5a (the permutation driver) against the stage 4
# SDFGs that ship with the AD. Always uses DaCe yakup/dev (no branch
# switching). For an F90 -> stage 4 regeneration, run
# tools/regenerate_baselines.sh first.

# Note: no `set -e` / `pipefail`. A crashed binary on one config (e.g.
# std::out_of_range from a missing data field) must not abort the whole
# batch -- subsequent configs still need to run. We handle setup
# failures explicitly where they matter (JSON missing, codegen fails).
set -u

# --verify retains the per-timestep got/want numerical-comparison blobs
# (O(GB) each) for offline validation against a reference run. Default
# is to delete them after each binary invocation.
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

# R02B05 / nproma=20480 dataset. tools/download_data.sh fetches into
# ${EXP_DIR}/data_r02b05/; idempotent (skips if non-empty).
[[ -d "${EXP_DIR}/data_r02b05" ]] && [[ -n "$(ls -A "${EXP_DIR}/data_r02b05" 2>/dev/null)" ]] \
    || bash tools/download_data.sh
export ICON_DATA_PATH="${ICON_DATA_PATH:-${EXP_DIR}/data_r02b05}"

# Layout configs to run. Default reproduces the paper's V1/V2/V6
# winner-comparison (§IV-D, Table IV) plus the lv-axis variants of
# nlev_first / index_only for the layout-conflict-resolution discussion:
#   winner_v1           V1 = h_first + SoA-conn (identity baseline)
#   winner_v2           V2 = h_first + AoS-conn (only connectivity permuted)
#   winner_v6           V6 = v_first + AoS-conn (the empirical winner)
#   unpermuted_lv*_sm*  baseline + map-shuffle ablations
#   nlev_first_lv*_sm*  v_first ablations (lv0 keeps levmask, lv1 transposes)
#   index_only_lv*_sm*  conn-only ablations
# Override at submission time, e.g.
#   CONFIGS="winner_v1 winner_v2 winner_v6" sbatch run_daint.sh
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

echo "[E7 daint] host=$(hostname) threads=$OMP_NUM_THREADS data=$ICON_DATA_PATH"
echo "[E7 daint] configs=$CONFIGS"
echo "[E7 daint] timesteps=$TIMESTEPS reps=$REPS warmup=$WARMUP"

# Stage 5a reads codegen/stage4/<variant>.sdfgz. The shipped stage 4 SDFGs
# live next to this script under SDFGs/stage4/; symlink them in place if
# tools/regenerate_baselines.sh has not been run.
if [[ ! -d "${EXP_DIR}/codegen/stage4" ]] && [[ -d "${EXP_DIR}/SDFGs/stage4" ]]; then
  mkdir -p "${EXP_DIR}/codegen"
  ln -sfn "${EXP_DIR}/SDFGs/stage4" "${EXP_DIR}/codegen/stage4"
fi

# E7 reads E6's access-analysis output (layout_candidates.json). Prefer
# the committed copy; if it's missing -- e.g. cluster checkout pulled
# only E7 -- regenerate it from the committed analysis.md.
LAYOUT_JSON="${EXP_DIR}/../E6_VelocityTendencies/access_analysis/layout_candidates.json"
if [[ ! -f "${LAYOUT_JSON}" ]]; then
  echo "[E7 daint] ${LAYOUT_JSON} missing; running select_loopnests.py"
  python "${EXP_DIR}/../E6_VelocityTendencies/access_analysis/select_loopnests.py"
fi

for CFG in ${CONFIGS}; do
  echo "[E7 daint] codegen + compile config=${CFG}"
  python -m utils.stages.stage5a --release --optimize --compile --config "${CFG}"

  bin="${EXP_DIR}/velocity_stage5a_${CFG}"
  if [[ ! -x "${bin}" ]]; then
    echo "[E7 daint] WARN: ${bin} not built; skipping run for ${CFG}" >&2
    continue
  fi

  # Run the binary once per timestep so each timestep lands in its
  # own output dir (results/daint/<config>/ts<N>/...) -- matches what
  # Figures/plot_paper_snapshot.sh expects. Stdout/stderr are tee'd to
  # ${out_dir}/run.txt so the slurm log keeps the live output AND the
  # per-timestep TXT records the same stream (icon-artifacts/velocity
  # convention -- a single TXT per (config, timestep) the plotting
  # script reads). When --verify is NOT passed, the got/want
  # numerical-comparison blobs (O(GB) per pair) are deleted after the
  # run; --verify keeps them for one-shot reference comparison.
  for TS in ${TIMESTEPS//,/ }; do
    out_dir="${EXP_DIR}/results/daint/${CFG}/ts${TS}"
    mkdir -p "${out_dir}"
    txt="${out_dir}/run.txt"
    echo "[E7 daint] running ${CFG} TS=${TS} reps=${REPS} warmup=${WARMUP} -> ${txt}"
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

# Mirror any CSVs that landed inside the per-config codegen tree (some
# pipeline steps emit them next to the SDFG instead of to --output-dir).
for d in "${EXP_DIR}/codegen/stage5a"/*/; do
  cfg="$(basename "${d}")"
  mkdir -p "${EXP_DIR}/results/daint/${cfg}"
  find "${d}" -maxdepth 2 -name '*.csv' -exec cp -v {} "${EXP_DIR}/results/daint/${cfg}/" \;
done

echo "[E7 daint] done. CSVs under results/daint/<config>/"
