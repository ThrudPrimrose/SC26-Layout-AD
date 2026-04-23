#!/usr/bin/env bash
# Delete benchmark-output CSVs under Experiments/. Intended to reset
# `results/` folders before a fresh sweep.
#
# Scope: every *.csv under Experiments/ EXCEPT the two safe-to-preserve
# paths below (Python venv site-packages test data and the DaCe clone,
# which ship CSVs that are nothing to do with our benchmarks).
#
# Usage:
#   bash clean_csvs.sh             # interactive prompt (default)
#   bash clean_csvs.sh --dry-run   # list files, don't delete
#   bash clean_csvs.sh -n          # ditto
#   bash clean_csvs.sh --yes       # skip the prompt
#   bash clean_csvs.sh -y          # ditto

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

dry_run=0
assume_yes=0
for arg in "$@"; do
    case "$arg" in
        -n|--dry-run) dry_run=1 ;;
        -y|--yes)     assume_yes=1 ;;
        -h|--help)
            sed -n '2,16p' "${BASH_SOURCE[0]}"
            exit 0 ;;
        *)  echo "[clean_csvs] unknown arg: $arg" >&2; exit 1 ;;
    esac
done

# Safety net: never touch venv / DaCe clone content.
mapfile -t targets < <(find "${SCRIPT_DIR}" -type f -name '*.csv' \
    -not -path '*/venv/*' -not -path '*/venv.*/*' -not -path '*/dace/*' \
    2>/dev/null | sort)

if (( ${#targets[@]} == 0 )); then
    echo "[clean_csvs] no benchmark CSVs found under ${SCRIPT_DIR}/"
    exit 0
fi

# Summary by top-level experiment folder.
echo "[clean_csvs] files to remove: ${#targets[@]}"
printf '%s\n' "${targets[@]}" \
  | awk -F/ -v root="${SCRIPT_DIR}" '
      { rel=$0; sub(root"/","",rel); split(rel, parts, "/");
        key = parts[1]; if (parts[1] ~ /^E6/ && parts[2] != "") key = parts[1]"/"parts[2];
        counts[key]++ }
      END { for (k in counts) printf "  %4d  %s\n", counts[k], k }' | sort -k2
size=$(printf '%s\n' "${targets[@]}" | xargs -r du -ch 2>/dev/null | tail -1 | awk '{print $1}')
echo "  total size: ${size:-unknown}"

if (( dry_run )); then
    echo "[clean_csvs] dry-run: no files removed."
    exit 0
fi

if (( ! assume_yes )); then
    read -r -p "[clean_csvs] delete these CSVs? [y/N] " reply
    case "${reply}" in
        y|Y|yes|YES) ;;
        *) echo "[clean_csvs] aborted."; exit 0 ;;
    esac
fi

deleted=0
for f in "${targets[@]}"; do
    rm -f -- "$f"
    deleted=$((deleted + 1))
done
echo "[clean_csvs] deleted ${deleted} CSV file(s)."
