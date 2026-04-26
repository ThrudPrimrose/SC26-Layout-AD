#!/usr/bin/env bash
#
# Fetch the serialized Quantum ESPRESSO BaTiO3 data that the E5 addusxx_g
# benchmark expects under ./bin/. The dataset is ~1 GiB uncompressed.
#
# The archive contains:
#   bin/addusxx_g__{qgm,eigts1,eigts2,eigts3,becphi_c,becpsi_c,rhoc,
#                   rhoc_out,ijtoh,mill,dfftt__nl}_<file_id>.bin
# and is consumed by main_cpu.cpp / main.cu / main_hip.cpp via
# ./bin/<fn> relative paths (see data_loading.h).
#
# The URL below points at the SPCL ETH PolyBox share that hosts the
# public copy used in the paper.

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

# TODO(AD reviewers): once the artifact is archived on Zenodo, replace
# this direct PolyBox link with the Zenodo DOI mirror.
URL="${USXX_DATA_URL:-https://polybox.ethz.ch/index.php/s/sBgGF5y4D2nbk25/download/bin_data_addusxx.tar.gz}"
ZIP="addusxx_g_bin_data.tar.gz"

if [[ -d bin && -n "$(ls -A bin 2>/dev/null)" ]]; then
  echo "[E5 download] bin/ already populated -- skip."
  exit 0
fi

echo "[E5 download] fetching $URL"
wget --no-verbose --show-progress -O "$ZIP" "$URL"

echo "[E5 download] unpacking $ZIP"
tar -xzf "$ZIP"
rm -f "$ZIP"

if [[ ! -d bin_data_addusxx ]]; then
  echo "[E5 download] ERROR: expected ./bin_data_addusxx/ directory not found after unzip" >&2
  exit 1
fi

# The archive unpacks into bin_data_addusxx/, but main_*.{cpp,cu,_hip.cpp}
# load via './bin/<fn>' paths. Rename the directory (not a symlink —
# real dir keeps everything portable under `cp -r`, `tar`, rsync, etc.).
if [[ ! -e bin ]]; then
  mv bin_data_addusxx bin
fi

echo "[E5 download] done -- $(ls bin | wc -l) files under bin/"