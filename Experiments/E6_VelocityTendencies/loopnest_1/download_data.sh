#!/usr/bin/env bash
# download_data.sh -- resolve an E6 loopnest's data_r02b05 directory.
#
# Priority chain (matches the one used by E8's run_*.sh, but biased
# toward E8 first since on a freshly-cloned tree E8 is what reviewers
# tend to run before E6):
#
#   1. ${TARGET} already populated         -> done (idempotent re-run).
#   2. E8/data_r02b05 populated            -> symlink ${TARGET} to it.
#   3. E7/data_r02b05 populated            -> symlink ${TARGET} to it.
#   4. Otherwise: fetch into E8/data_r02b05 via E7's
#      ``tools/download_data.sh`` (the canonical 9 GB R02B05 fetcher),
#      then symlink ${TARGET} to it.
#
# Argument: the loopnest's target path (where the symlink should end
# up). Defaults to ``${SCRIPT_DIR}/data_r02b05`` (i.e. loopnest_1's).
#
# Idempotent. Safe to call from any loopnest's run script.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET="${1:-${SCRIPT_DIR}/data_r02b05}"

E6_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
EXPERIMENTS_ROOT="$(cd "${E6_ROOT}/.." && pwd)"

E8_DATA_DIR="${EXPERIMENTS_ROOT}/E8_LegacyVT/data_r02b05"
E7_DATA_DIR="${EXPERIMENTS_ROOT}/E7_FullVelocityTendencies/data_r02b05"
DOWNLOAD_DATA_SH="${EXPERIMENTS_ROOT}/E7_FullVelocityTendencies/tools/download_data.sh"

_have_data() {
    [[ -d "$1" ]] && [[ -n "$(ls -A "$1" 2>/dev/null)" ]]
}

# --- 1. already in place ------------------------------------------------
if _have_data "${TARGET}"; then
    echo "[E6 download_data] ${TARGET} already populated; nothing to do"
    exit 0
fi

# Stale broken symlink / empty stub? Remove it so we can replace it.
if [[ -L "${TARGET}" || -e "${TARGET}" ]]; then
    rm -f  "${TARGET}" 2>/dev/null || true
    rmdir  "${TARGET}" 2>/dev/null || true
fi
mkdir -p "$(dirname "${TARGET}")"

# --- 2. E8 has it -------------------------------------------------------
if _have_data "${E8_DATA_DIR}"; then
    echo "[E6 download_data] symlinking ${TARGET} -> ${E8_DATA_DIR}"
    ln -sfn "${E8_DATA_DIR}" "${TARGET}"
    exit 0
fi

# --- 3. E7 has it -------------------------------------------------------
if _have_data "${E7_DATA_DIR}"; then
    echo "[E6 download_data] symlinking ${TARGET} -> ${E7_DATA_DIR}"
    ln -sfn "${E7_DATA_DIR}" "${TARGET}"
    exit 0
fi

# --- 4. fetch into E8, then symlink -------------------------------------
if [[ ! -f "${DOWNLOAD_DATA_SH}" ]]; then
    echo "[E6 download_data] ERROR: downloader not found at ${DOWNLOAD_DATA_SH}" >&2
    exit 1
fi

echo "[E6 download_data] no existing data; fetching into ${E8_DATA_DIR}"
mkdir -p "$(dirname "${E8_DATA_DIR}")"
OUTPUT_DIR="${E8_DATA_DIR}" bash "${DOWNLOAD_DATA_SH}"

echo "[E6 download_data] symlinking ${TARGET} -> ${E8_DATA_DIR}"
ln -sfn "${E8_DATA_DIR}" "${TARGET}"
