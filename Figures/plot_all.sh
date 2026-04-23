#!/usr/bin/env bash
# Regenerate every figure the paper needs.
#
# Illustrative groups -- pure-matplotlib scripts under Figures/<group>/:
#   AccessCost, Pebble_Game, LayoutTransformations, Replay
#   These save with relative paths like plt.savefig('name.pdf'), so we
#   `cd` into Figures/GeneratedFigures/<group>/ before running.
#
# Runtime figures -- split across two dedicated sibling scripts:
#   PaperSnapshot  -> Figures/plot_paper_snapshot.sh
#                     (plots the frozen paper-canonical CSVs committed
#                      under PaperSnapshot/<exp>/results/; outputs land
#                      in Figures/GeneratedFigures/Runtime/)
#   Results        -> Figures/plot_results.sh
#                     (plots the reviewer's locally-produced CSVs in
#                      Experiments/<exp>/results/ after sbatch;
#                      outputs land next to the CSVs under the same
#                      Experiments/<exp>/results/ directory)
#   Runtime        -> invokes both of the above in sequence (default).
#
# Peaks group -- refreshes the canonical STREAM-peak JSON consumed by
#   every plot_paper.py for bandwidth normalization (no figures
#   produced; Experiments/E0_NUMA/plot_paper.py overwrites
#   Experiments/common/stream_peak.json from measured CSVs when
#   available, and falls back to the hardcoded defaults otherwise).
#
# Run with no args to regenerate all groups, or pass a subset:
#     bash plot_all.sh AccessCost Runtime
#     bash plot_all.sh Peaks             # just refresh stream_peak.json
#     bash plot_all.sh PaperSnapshot     # just the frozen paper figures
#     bash plot_all.sh Results           # just your reviewer-generated figures
#     bash plot_all.sh Runtime           # both PaperSnapshot + Results

set -euo pipefail

FIG_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${FIG_DIR}/.." && pwd)"
EXP_ROOT="${REPO_ROOT}/Experiments"
OUT_ROOT="${FIG_DIR}/GeneratedFigures"

# group -> space-separated list of python files (relative to the group dir).
# unset first in case the script is sourced into an env where GROUPS is
# already a plain indexed array (bash 5.x warns otherwise).
unset GROUPS 2>/dev/null || true
declare -gA GROUPS=(
    [AccessCost]="block_layouts.py plot_blocks_touched.py plot_access_cost_row_major.py plot_access_cost_tiled.py plot_access_cost_numa.py"
    [Pebble_Game]="pebble.py pebble_game_1.py pebble_game_2.py"
    [LayoutTransformations]="tiled_addition.py pack_unpack.py layout_stages.py"
    [Replay]="replay.py"
)

DEFAULT_ORDER=(AccessCost Pebble_Game LayoutTransformations Replay Peaks Runtime)

if (( $# == 0 )); then
    targets=("${DEFAULT_ORDER[@]}")
else
    targets=("$@")
fi

for group in "${targets[@]}"; do
    case "${group}" in
      Peaks)
        echo "[Peaks] refreshing ${EXP_ROOT}/common/stream_peak.json"
        if [[ -f "${EXP_ROOT}/E0_NUMA/plot_paper.py" ]]; then
            ( cd "${EXP_ROOT}/E0_NUMA" && python plot_paper.py ) \
                || echo "  [warn] E0_NUMA/plot_paper.py failed"
        else
            echo "  [skip] E0_NUMA/plot_paper.py not found"
        fi
        ;;

      PaperSnapshot)
        bash "${FIG_DIR}/plot_paper_snapshot.sh"
        ;;

      Results)
        bash "${FIG_DIR}/plot_results.sh"
        ;;

      Runtime)
        bash "${FIG_DIR}/plot_paper_snapshot.sh"
        bash "${FIG_DIR}/plot_results.sh"
        ;;

      *)
        scripts="${GROUPS[${group}]:-}"
        if [[ -z "${scripts}" ]]; then
            echo "[skip] unknown group: ${group}" >&2
            continue
        fi
        out="${OUT_ROOT}/${group}"
        mkdir -p "${out}"
        echo "[${group}] writing to ${out}"
        for s in ${scripts}; do
            src="${FIG_DIR}/${group}/${s}"
            if [[ ! -f "${src}" ]]; then
                echo "  [skip] missing ${src}" >&2
                continue
            fi
            echo "  python ${s}"
            if ! ( cd "${out}" && python "${src}" ); then
                echo "  [warn] ${group}/${s} failed" >&2
            fi
        done
        ;;
    esac
done

echo
echo "done."
