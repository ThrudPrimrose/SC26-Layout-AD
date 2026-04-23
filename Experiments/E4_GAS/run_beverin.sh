#!/usr/bin/env bash
#SBATCH --job-name=E4_zaxpy_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=02:00:00
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=192
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/beverin/E4_zaxpy_beverin_%j.out
#SBATCH --error=results/beverin/E4_zaxpy_beverin_%j.err
#
# E4 Gather-Accumulate-Scatter (Figure 10) on Beverin.

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"
cd "${EXP_DIR}"

[[ -d bin ]] || bash download_data.sh

echo "[E4 beverin] host=$(hostname) threads=$OMP_NUM_THREADS"

# --- build (flags from common/setup_beverin.sh) --------------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o zaxpy_cpu zaxpy.cpp     ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o zaxpy_gpu zaxpy_hip.cpp ${GPU_LDFLAGS}

# --- run (binaries write CSV at the paths passed as argv[1] / argv[2]) ---
./zaxpy_cpu results/beverin/zaxpy_sweep_small_cpu.csv results/beverin/zaxpy_sweep_1gb_cpu.csv
./zaxpy_gpu results/beverin/zaxpy_sweep_small.csv     results/beverin/zaxpy_sweep_1gb.csv

echo "[E4 beverin] done. CSVs under results/beverin/"
