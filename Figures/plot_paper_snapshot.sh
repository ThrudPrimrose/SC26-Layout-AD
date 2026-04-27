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
# E8 (legacy stage-8 pipeline) is the default §IV-D / Fig 14 / Tab V
# path; E7 is opportunistic. ``has_snapshot_evidence`` accepts both
# the canonical ``results/{*.csv,*.txt}`` shape and E8's native
# ``{daint,beverin}_full_permutations_8/*.txt`` shape (placed at the
# snapshot dir's root, not under ``results/``).
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

has_snapshot_evidence() {
    local d="$1"
    [[ -d "$d/results" ]] && {
        find "$d/results" -type f \( -name '*.csv' -o -name '*.txt' \) -print -quit \
            | grep -q . && return 0
    }
    find "$d" -maxdepth 3 -path '*_full_permutations_8/*.txt' -print -quit 2>/dev/null \
        | grep -q . && return 0
    return 1
}

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
    if ! has_snapshot_evidence "${snap_dir}"; then
        echo "  [skip] ${exp}: PaperSnapshot/${exp}/ has no CSVs / TXTs / *_full_permutations_8/*.txt"
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
