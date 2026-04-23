#!/usr/bin/env bash
#SBATCH --job-name=E4_zaxpy_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=01:00:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=288
#SBATCH --output=results/daint/E4_zaxpy_daint_%j.out
#SBATCH --error=results/daint/E4_zaxpy_daint_%j.err
#
# E4 Gather-Accumulate-Scatter (Figure 10) on Daint.Alps.

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"
cd "${EXP_DIR}"

[[ -d bin ]] || bash download_data.sh

echo "[E4 daint] host=$(hostname) threads=$OMP_NUM_THREADS"

# --- build (flags from common/setup_daint.sh) ----------------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o zaxpy_cpu zaxpy.cpp ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o zaxpy_gpu zaxpy.cu  ${GPU_LDFLAGS}

# --- run (binaries write CSV at the paths passed as argv[1] / argv[2]) ---
./zaxpy_cpu results/daint/zaxpy_sweep_small_cpu.csv results/daint/zaxpy_sweep_1gb_cpu.csv
./zaxpy_gpu results/daint/zaxpy_sweep_small.csv     results/daint/zaxpy_sweep_1gb.csv

echo "[E4 daint] done. CSVs under results/daint/"
