#!/usr/bin/env bash
# clean_generated_artifacts.sh -- drop *.pdf and *.png under Experiments/
#
# Mirrors the .err-cleanup pattern, scoped to Experiments/ only:
#   * stage deletion of every tracked Experiments/**/*.pdf or *.png,
#   * delete any untracked ones from disk,
#   * append the matching patterns to root .gitignore so future builds
#     don't accidentally re-track them.
#
# Stays clear of Figures/GeneratedFigures/, Latex/, and PaperSnapshot/
# (paper-snapshot artefacts that must remain tracked).
#
# Usage:
#   ./clean_generated_artifacts.sh            # dry-run
#   ./clean_generated_artifacts.sh --apply    # do it
#   ./clean_generated_artifacts.sh --apply --commit
#
set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT"

APPLY=0
COMMIT=0
for arg in "$@"; do
  case "$arg" in
    --apply)  APPLY=1 ;;
    --commit) COMMIT=1 ;;
    -h|--help) sed -n '2,17p' "${BASH_SOURCE[0]}"; exit 0 ;;
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

# --- Step 1: stage deletion of tracked Experiments/**/*.{pdf,png} ---
tracked_removed=0
while IFS= read -r f; do
  [[ -z "$f" ]] && continue
  run git rm -f -q -- "$f"
  : $((tracked_removed++))
done < <(git ls-files -- 'Experiments/**/*.pdf' 'Experiments/**/*.png' 2>/dev/null)

# --- Step 2: delete untracked Experiments/**/*.{pdf,png} on disk ---
untracked_removed=0
while IFS= read -r f; do
  if git ls-files --error-unmatch -- "$f" >/dev/null 2>&1; then
    continue   # already handled in step 1
  fi
  run rm -f -- "$f"
  : $((untracked_removed++))
done < <(find Experiments -type f \( -name '*.pdf' -o -name '*.png' \) 2>/dev/null)

# --- Step 3: ensure root .gitignore has scoped ignore patterns ---
gitignore_added=0
for pat in 'Experiments/**/*.pdf' 'Experiments/**/*.png'; do
  if ! grep -qxF "$pat" .gitignore 2>/dev/null; then
    if (( APPLY )); then
      printf '%s\n' "$pat" >> .gitignore
    else
      printf '[dry-run] echo %q >> .gitignore\n' "$pat"
    fi
    : $((gitignore_added++))
  fi
done
if (( gitignore_added > 0 )) && (( APPLY )); then
  run git add .gitignore
fi

# --- Step 4: optionally commit ---
if (( COMMIT )); then
  if git diff --cached --quiet; then
    echo "(nothing staged; skipping commit)"
  else
    run git commit -m "Drop Experiments/**/*.{pdf,png}; add to .gitignore"
  fi
fi

echo
echo "summary:"
echo "  tracked .pdf/.png removed   : ${tracked_removed}"
echo "  untracked .pdf/.png removed : ${untracked_removed}"
echo "  .gitignore patterns added   : ${gitignore_added}"
if (( ! APPLY )); then
  echo
  echo "(dry-run -- pass --apply to actually run; --commit to also commit)"
fi
