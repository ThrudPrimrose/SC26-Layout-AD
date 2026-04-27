#!/usr/bin/env bash
# plot_results.sh -- regenerate the runtime figures (Figures 4, 8-11)
# from YOUR locally-produced CSVs.
#
# Prerequisite: you ran sbatch run_{daint,beverin}.sh in each experiment
# you want to re-plot, so Experiments/<exp>/results/{daint,beverin}/*.csv
# is populated.
#
# For every Runtime experiment (E1..E5):
#   1. cd Experiments/<exp>/   (cwd-relative paths resolve to results/*.csv)
#   2. python plot_paper.py
#   3. move every .pdf / .png written by step 2 into
#      Experiments/<exp>/results/   (next to the CSVs that drove them)
#
# The produced PDFs/PNGs end up co-located with the data:
#     Experiments/<exp>/results/<stem>.{pdf,png}
# This keeps reviewer-generated figures out of Figures/GeneratedFigures/
# (which is reserved for the frozen paper snapshot produced by
# plot_paper_snapshot.sh).
#
# An experiment with no CSVs in results/ is skipped with a one-line
# notice; run its sbatch script first.
#
# Usage:  bash Figures/plot_results.sh

set -euo pipefail

FIG_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${FIG_DIR}/.." && pwd)"
EXP_ROOT="${REPO_ROOT}/Experiments"

# Pin every figure to DejaVu Sans; avoids STIX / Computer Modern fallbacks
# that flood the log with font-not-found warnings on stock environments.
export MATPLOTLIBRC="${FIG_DIR}/matplotlibrc"

# Paths are relative to Experiments/ so deeper experiments (e.g. E6
# loopnest_N) slot in without special-casing.
# E8 is the default full-velocity-tendencies path (paper §IV-D / Fig 14
# / Tab V); E7 is opportunistic. Everything else lists in source-paper
# order. ``has_runtime_evidence`` picks the right per-experiment
# evidence shape -- E8 emits ``<plat>_full_permutations_8/*.txt`` at
# the experiment root, not under ``results/``.
RUNTIME_EXPS=(
  E1_MatrixAdd
  E2_Conjugation
  E3_Transpose
  E4_GAS
  E5_USXX
  E6_VelocityTendencies/loopnest_1
  E6_VelocityTendencies/loopnest_2
  E6_VelocityTendencies/loopnest_3
  E6_VelocityTendencies/loopnest_4
  E6_VelocityTendencies/loopnest_5
  E6_VelocityTendencies/loopnest_6
  E8_LegacyVT
  E7_FullVelocityTendencies
)

# Returns 0 iff ``$1`` (an experiment dir) has anything plot-worthy.
# E8 keeps results in ``{daint,beverin}_full_permutations_8/*.txt`` next
# to the run scripts, NOT under ``results/``; everything else (E1-E7)
# uses the canonical ``results/<platform>/.../{*.csv,run.txt}`` shape.
has_runtime_evidence() {
    local d="$1"
    [[ -d "$d/results" ]] && {
        find "$d/results" -type f \( -name '*.csv' -o -name 'run.txt' \) -print -quit \
            | grep -q . && return 0
    }
    find "$d" -maxdepth 2 -type d -name '*_full_permutations_8' -print -quit 2>/dev/null \
        | grep -q . && {
        find "$d" -maxdepth 3 -path '*_full_permutations_8/*.txt' -print -quit 2>/dev/null \
            | grep -q . && return 0
    }
    return 1
}

marker="$(mktemp /tmp/plot_results_marker.XXXXXX)"
echo "[plot_results] figures will land in each Experiments/<exp>/results/ (or, for E8, next to <plat>_full_permutations_8/)"
for exp in "${RUNTIME_EXPS[@]}"; do
    exp_dir="${EXP_ROOT}/${exp}"
    # Every experiment ships plot_paper.py (E7's was the former
    # plot_paper_v2.py; both v1/v2 logic lives in the single file now).
    script_name="plot_paper.py"
    if [[ ! -f "${exp_dir}/${script_name}" ]]; then
        echo "  [skip] ${exp}: no ${script_name}"
        continue
    fi
    if ! has_runtime_evidence "${exp_dir}"; then
        echo "  [skip] ${exp}: no CSVs / run.txt / *_full_permutations_8/*.txt (run sbatch first)"
        continue
    fi
    # E8 lands its own figures next to the .txt logs at the experiment
    # root; everyone else routes them into results/.
    if [[ -d "${exp_dir}/results" ]]; then
        out="${exp_dir}/results"
    else
        out="${exp_dir}"
    fi
    echo "  [plot] ${exp} -> ${out#${REPO_ROOT}/}/  (${script_name})"
    touch "${marker}"
    if ! ( cd "${exp_dir}" && python "${script_name}" ); then
        echo "  [warn] ${exp}/${script_name} failed" >&2
        continue
    fi
    moved=0
    while IFS= read -r -d '' f; do
        mv "${f}" "${out}/"
        moved=$((moved+1))
    done < <(find "${exp_dir}" -maxdepth 1 \
                 \( -name '*.pdf' -o -name '*.png' \) \
                 -newer "${marker}" -print0)
    echo "    -> ${moved} figure(s) into ${out#${REPO_ROOT}/}/"
done
rm -f "${marker}"
echo
echo "done."
