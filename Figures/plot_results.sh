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
)

marker="$(mktemp /tmp/plot_results_marker.XXXXXX)"
echo "[plot_results] figures will land in each Experiments/<exp>/results/"
for exp in "${RUNTIME_EXPS[@]}"; do
    exp_dir="${EXP_ROOT}/${exp}"
    script="${exp_dir}/plot_paper.py"
    if [[ ! -f "${script}" ]]; then
        echo "  [skip] ${exp}: no plot_paper.py"
        continue
    fi
    if [[ ! -d "${exp_dir}/results" ]] \
       || [[ -z "$(find "${exp_dir}/results" -type f -name '*.csv' -print -quit)" ]]; then
        echo "  [skip] ${exp}: Experiments/${exp}/results/ has no CSVs (run sbatch first)"
        continue
    fi
    out="${exp_dir}/results"
    echo "  [plot] ${exp} -> ${out#${REPO_ROOT}/}/"
    touch "${marker}"
    if ! ( cd "${exp_dir}" && python plot_paper.py ); then
        echo "  [warn] ${exp}/plot_paper.py failed" >&2
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
