#!/usr/bin/env bash
# clean_core_dumps.sh -- recursively remove core_nid* dumps under Experiments/.
#
# Usage:
#   ./clean_core_dumps.sh              # dry-run: list matches and total size
#   ./clean_core_dumps.sh --delete     # prompt, then delete
#   ./clean_core_dumps.sh --delete -y  # delete without prompting
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DELETE=0
YES=0
for arg in "$@"; do
  case "$arg" in
    -f|--delete)  DELETE=1 ;;
    -y|--yes)     YES=1 ;;
    -h|--help)
      sed -n '2,8p' "${BASH_SOURCE[0]}"; exit 0 ;;
    *) echo "unknown arg: $arg" >&2; exit 2 ;;
  esac
done

count=$(find "$ROOT" -type f -name 'core_nid*' -printf '.' 2>/dev/null | wc -c)
if [[ "$count" -eq 0 ]]; then
  echo "no core_nid* files under $ROOT"
  exit 0
fi

bytes=$(find "$ROOT" -type f -name 'core_nid*' -printf '%s\n' | awk '{s+=$1} END{print s+0}')
human=$(numfmt --to=iec --suffix=B "$bytes" 2>/dev/null || echo "${bytes} B")

echo "found $count core_nid* files, total $human, under $ROOT"

if [[ "$DELETE" -eq 0 ]]; then
  echo "(dry-run -- pass --delete to remove)"
  exit 0
fi

if [[ "$YES" -eq 0 ]]; then
  read -r -p "delete these files? [y/N] " reply
  [[ "$reply" =~ ^[Yy]$ ]] || { echo "aborted"; exit 1; }
fi

find "$ROOT" -type f -name 'core_nid*' -delete
echo "deleted $count files ($human reclaimed)"
