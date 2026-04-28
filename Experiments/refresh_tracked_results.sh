#!/usr/bin/env bash
# refresh_tracked_results.sh -- prune stale slurm logs and force-track results
#
# For every `<stem>_<jobid>.{err,out}` under Experiments/, keep only the
# file with the largest <jobid> per (directory, stem, extension); remove
# (`git rm` if tracked, plain `rm` otherwise) the older ones. Then force-add
# the surviving log + every `*.csv` under Experiments/<E>*/results/ so they
# land in the index even when root-level .gitignore would normally hide
# them (`*.out`, `Experiments/*/results/`).
#
# Usage:
#   ./refresh_tracked_results.sh            # dry-run: list what would change
#   ./refresh_tracked_results.sh --apply    # do it
#
set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT"

APPLY=0
for arg in "$@"; do
  case "$arg" in
    --apply) APPLY=1 ;;
    -h|--help) sed -n '2,14p' "${BASH_SOURCE[0]}"; exit 0 ;;
    *) echo "unknown arg: $arg" >&2; exit 2 ;;
  esac
done

run() {
  if (( APPLY )); then
    "$@"
  else
    printf '[dry-run] %s\n' "$*"
  fi
}

# Returns 0 if `$1` is tracked AND its working-tree content matches HEAD
# (i.e. `git add` would be a no-op). 1 otherwise.
is_tracked_and_clean() {
  git ls-files --error-unmatch -- "$1" >/dev/null 2>&1 || return 1
  git diff --quiet HEAD -- "$1" 2>/dev/null
}

# --- Step 1: group .err/.out files by (dir, stem, ext); keep largest jobid ---
declare -A KEEP_FILE   # key -> path of current best
declare -A KEEP_ID     # key -> largest jobid seen

removed_old=0
kept=0

while IFS= read -r f; do
  base="${f##*/}"
  if [[ "$base" =~ ^(.+)_([0-9]+)\.(err|out)$ ]]; then
    stem_dir="$(dirname "$f")"
    stem_name="${BASH_REMATCH[1]}"
    jobid="${BASH_REMATCH[2]}"
    ext="${BASH_REMATCH[3]}"
    key="${stem_dir}|${stem_name}|${ext}"
    cur_id="${KEEP_ID[$key]:-0}"
    if (( jobid > cur_id )); then
      # Demote previous winner.
      prev="${KEEP_FILE[$key]:-}"
      if [[ -n "$prev" ]]; then
        if git ls-files --error-unmatch -- "$prev" >/dev/null 2>&1; then
          run git rm -f -q -- "$prev"
        else
          run rm -f -- "$prev"
        fi
        : $((removed_old++))
      fi
      KEEP_FILE[$key]="$f"
      KEEP_ID[$key]="$jobid"
    else
      if git ls-files --error-unmatch -- "$f" >/dev/null 2>&1; then
        run git rm -f -q -- "$f"
      else
        run rm -f -- "$f"
      fi
      : $((removed_old++))
    fi
  fi
done < <(find Experiments -type f \( -name '*.err' -o -name '*.out' \) 2>/dev/null)

# --- Step 2: force-add the surviving slurm logs (skip already-clean) ---
kept_skipped=0
for f in "${KEEP_FILE[@]}"; do
  if is_tracked_and_clean "$f"; then
    : $((kept_skipped++))
    continue
  fi
  run git add -f -- "$f"
  : $((kept++))
done

# --- Step 3: force-add every CSV under Experiments/.../results/ (skip clean) ---
csv_added=0
csv_skipped=0
while IFS= read -r f; do
  if is_tracked_and_clean "$f"; then
    : $((csv_skipped++))
    continue
  fi
  run git add -f -- "$f"
  : $((csv_added++))
done < <(find Experiments -type f -name '*.csv' -path '*/results/*' 2>/dev/null)

echo
echo "summary:"
echo "  older slurm logs removed     : ${removed_old}"
echo "  latest slurm logs force-added: ${kept}  (already-clean skipped: ${kept_skipped})"
echo "  CSVs force-added             : ${csv_added}  (already-clean skipped: ${csv_skipped})"
if (( ! APPLY )); then
  echo
  echo "(dry-run -- pass --apply to actually run)"
fi
