#!/usr/bin/env bash
# Fetch the E4 GAS indirect-access data into ./bin/. Idempotent: skips
# when bin/ is already populated with .bin files.

set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

URL="${E4_DATA_URL:-https://polybox.ethz.ch/index.php/s/ySAEwfRTSH4Jzmr/download/bin_indirect_accesses-20260406T132129Z-1-001.zip}"
ZIP="bin_indirect_accesses.zip"

shopt -s nullglob
existing=(bin/*.bin)
shopt -u nullglob
if (( ${#existing[@]} > 0 )); then
  echo "[E4 download] bin/ already populated (${#existing[@]} .bin files) -- skip."
  exit 0
fi

echo "[E4 download] fetching $URL"
wget --no-verbose -O "$ZIP" "$URL"

echo "[E4 download] unpacking $ZIP"
unzip -q -o "$ZIP"
rm -f "$ZIP"

mkdir -p bin
if [[ -d bin_indirect_accesses ]]; then
  mv bin_indirect_accesses/* bin/ 2>/dev/null || true
  rmdir bin_indirect_accesses 2>/dev/null || true
fi

echo "[E4 download] done."
