#!/usr/bin/env bash
# Regenerate every illustrative / supplemental figure.
#
# Each subfolder under Figures/ holds a small group of matplotlib /
# tikz-ish plot generators. The scripts save with relative paths like
# `plt.savefig('name.pdf')`, so this wrapper `cd`s into the matching
# GeneratedFigures/<group>/ folder before running each script. That way
# every PDF/PNG lands in one place, not next to the source.
#
# Run with no args to regenerate all groups; pass group names to regen
# a subset, e.g.:
#     bash plot_all.sh AccessCost Pebble_Game

set -euo pipefail

FIG_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_ROOT="${FIG_DIR}/GeneratedFigures"

# group -> space-separated list of python files (relative to the group dir).
declare -A GROUPS=(
    [AccessCost]="block_layouts.py plot_blocks_touched.py plot_access_cost_row_major.py plot_access_cost_tiled.py plot_access_cost_numa.py"
    [Pebble_Game]="pebble.py pebble_game_1.py pebble_game_2.py"
    [LayoutTransformations]="tiled_addition.py pack_unpack.py layout_stages.py"
    [Replay]="replay.py"
)
DEFAULT_ORDER=(AccessCost Pebble_Game LayoutTransformations Replay)

if (( $# == 0 )); then
    targets=("${DEFAULT_ORDER[@]}")
else
    targets=("$@")
fi

for group in "${targets[@]}"; do
    scripts="${GROUPS[$group]:-}"
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
        ( cd "${out}" && python "${src}" )
    done
done

echo
echo "done. outputs under ${OUT_ROOT}/"
