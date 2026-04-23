#!/usr/bin/env bash
# Regenerate every figure (illustrative + runtime) the paper needs.
#
# Illustrative groups -- pure-matplotlib scripts under Figures/<group>/:
#   AccessCost, Pebble_Game, LayoutTransformations, Replay
#   These save with relative paths like plt.savefig('name.pdf'), so we
#   `cd` into Figures/GeneratedFigures/<group>/ before running.
#
# Runtime group -- the paper's per-experiment plots (Fig. 4, 8-11):
#   runs each Experiments/<exp>/plot_paper.py twice:
#     1. from inside PaperSnapshot/<exp>/ (if populated) -> the paper-
#        canonical figures land flat in Figures/GeneratedFigures/Runtime/
#     2. from inside Experiments/<exp>/ (always) -> the fresh local
#        reproduction lands flat in Figures/GeneratedFigures/Runtime/new/
#   Because plot_paper.py reads CSVs via cwd-relative paths like
#   results/{daint,beverin}/*.csv, pointing cwd at PaperSnapshot/<exp>/
#   is all it takes to plot from the frozen paper data instead of the
#   live local results. An empty PaperSnapshot/<exp>/results/ tree (only
#   .gitkeep) causes that experiment's paper step to be skipped; the
#   new-reproduction step still runs.
#
# Peaks group -- refreshes the canonical STREAM-peak JSON consumed by
#   every plot_paper.py for bandwidth normalization (no figures
#   produced; Experiments/E0_NUMA/plot_paper.py overwrites
#   Experiments/common/stream_peak.json from measured CSVs when
#   available, and falls back to the hardcoded defaults otherwise).
#
# Run with no args to regenerate all groups, or pass a subset:
#     bash plot_all.sh AccessCost Runtime
#     bash plot_all.sh Peaks        # just refresh stream_peak.json
#     bash plot_all.sh Runtime      # just runtime figures

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

# Experiments that produce runtime figures (Peaks is E0, handled separately).
RUNTIME_EXPS=(E1_MatrixAdd E2_Conjugation E3_Transpose E4_GAS E5_USXX)

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

      Runtime)
        out_paper="${OUT_ROOT}/Runtime"
        out_new="${OUT_ROOT}/Runtime/new"
        mkdir -p "${out_paper}" "${out_new}"
        echo "[Runtime] paper -> ${out_paper}"
        echo "[Runtime] new   -> ${out_new}"
        marker="$(mktemp /tmp/plot_all_marker.XXXXXX)"
        snap_root="${REPO_ROOT}/PaperSnapshot"
        for exp in "${RUNTIME_EXPS[@]}"; do
            exp_dir="${EXP_ROOT}/${exp}"
            snap_dir="${snap_root}/${exp}"
            script="${exp_dir}/plot_paper.py"
            if [[ ! -f "${script}" ]]; then
                echo "  [skip] ${exp}: no plot_paper.py" >&2
                continue
            fi

            # ---- Paper-canonical step (if snapshot has any CSVs). ----
            if [[ -d "${snap_dir}/results" ]] \
               && [[ -n "$(find "${snap_dir}/results" -type f -name '*.csv' -print -quit)" ]]; then
                echo "  [paper] ${exp}"
                touch "${marker}"
                if ( cd "${snap_dir}" && python "${script}" ); then
                    moved=0
                    while IFS= read -r -d '' f; do
                        mv "${f}" "${out_paper}/"
                        moved=$((moved+1))
                    done < <(find "${snap_dir}" -maxdepth 1 \
                                 \( -name '*.pdf' -o -name '*.png' \) \
                                 -newer "${marker}" -print0)
                    echo "    -> ${moved} figure(s) into ${out_paper}/"
                else
                    echo "  [warn] ${exp} paper-snapshot plot failed" >&2
                fi
            else
                echo "  [skip] ${exp} paper snapshot empty (populate PaperSnapshot/${exp}/results/)"
            fi

            # ---- Fresh local-reproduction step (always, if CSVs exist). ----
            echo "  [new]   ${exp}"
            touch "${marker}"
            if ( cd "${exp_dir}" && python "${script}" ); then
                moved=0
                while IFS= read -r -d '' f; do
                    mv "${f}" "${out_new}/"
                    moved=$((moved+1))
                done < <(find "${exp_dir}" -maxdepth 1 \
                             \( -name '*.pdf' -o -name '*.png' \) \
                             -newer "${marker}" -print0)
                echo "    -> ${moved} figure(s) into ${out_new}/"
            else
                echo "  [warn] ${exp} local reproduction plot failed" >&2
            fi
        done
        rm -f "${marker}"
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
echo "done. outputs under ${OUT_ROOT}/"
