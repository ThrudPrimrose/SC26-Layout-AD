#!/usr/bin/env bash
# plot_paper_snapshot.sh -- regenerate the paper-canonical runtime
# figures (Figures 4, 8-11) from the frozen CSV snapshot committed
# alongside the artifact.
#
# For every Runtime experiment (E1..E5):
#   1. cd PaperSnapshot/<exp>/   (mirrors Experiments/<exp>/results/ layout
#                                 so plot_paper.py's cwd-relative CSV paths
#                                 resolve unchanged).
#   2. python Experiments/<exp>/plot_paper.py
#   3. move every .pdf / .png written by step 2 into
#      Figures/GeneratedFigures/Runtime/.
#
# This is the sibling of plot_results.sh, which does the same thing but
# from Experiments/<exp>/results/ (the reviewer's own CSVs) and writes
# the figures next to the data instead of into Figures/GeneratedFigures/.
#
# Usage:  bash Figures/plot_paper_snapshot.sh

set -euo pipefail

FIG_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${FIG_DIR}/.." && pwd)"
SNAP_ROOT="${REPO_ROOT}/PaperSnapshot"
EXP_ROOT="${REPO_ROOT}/Experiments"
OUT_DIR="${FIG_DIR}/GeneratedFigures/Runtime"

# Pin every figure to DejaVu Sans; avoids STIX / Computer Modern fallbacks
# that flood the log with font-not-found warnings on stock environments.
export MATPLOTLIBRC="${FIG_DIR}/matplotlibrc"

# Paths are relative to Experiments/ and PaperSnapshot/ so deeper
# experiments (e.g. E6 loopnest_N) slot in without special-casing.
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
  E7_FullVelocityTendencies
)

mkdir -p "${OUT_DIR}"
echo "[plot_paper_snapshot] writing to ${OUT_DIR}"
marker="$(mktemp /tmp/plot_snap_marker.XXXXXX)"
for exp in "${RUNTIME_EXPS[@]}"; do
    snap_dir="${SNAP_ROOT}/${exp}"
    script="${EXP_ROOT}/${exp}/plot_paper.py"
    if [[ ! -f "${script}" ]]; then
        echo "  [skip] ${exp}: no plot_paper.py at Experiments/${exp}/"
        continue
    fi
    if [[ ! -d "${snap_dir}/results" ]] \
       || [[ -z "$(find "${snap_dir}/results" -type f \( -name '*.csv' -o -name '*.txt' \) -print -quit)" ]]; then
        # E7 emits stdout-derived timing TXTs (icon-artifacts/velocity
        # convention -- ``run.txt`` per (config, timestep)) alongside
        # any DaCe-profiling CSVs; either one is sufficient evidence
        # the experiment was actually run.
        echo "  [skip] ${exp}: PaperSnapshot/${exp}/results/ has no CSVs or TXTs"
        continue
    fi
    echo "  [plot] ${exp}"
    touch "${marker}"
    if ! ( cd "${snap_dir}" && python "${script}" ); then
        echo "  [warn] ${exp}/plot_paper.py failed" >&2
        continue
    fi
    moved=0
    while IFS= read -r -d '' f; do
        mv "${f}" "${OUT_DIR}/"
        moved=$((moved+1))
    done < <(find "${snap_dir}" -maxdepth 1 \
                 \( -name '*.pdf' -o -name '*.png' \) \
                 -newer "${marker}" -print0)
    echo "    -> ${moved} figure(s) into ${OUT_DIR#${REPO_ROOT}/}/"
done
rm -f "${marker}"
echo
echo "done. paper-canonical runtime figures under ${OUT_DIR#${REPO_ROOT}/}/"
