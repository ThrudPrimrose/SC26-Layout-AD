#!/usr/bin/env bash
#SBATCH --job-name=E5_addusxx_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=05:00:00
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=192
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/beverin/E5_addusxx_beverin_%j.out
#SBATCH --error=results/beverin/E5_addusxx_beverin_%j.err
#
# E5 USXX (addusxx_g) AoS vs SoA sweep on Beverin.

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"
cd "${EXP_DIR}"

[[ -d bin ]] || bash download_data.sh

echo "[E5 beverin] host=$(hostname) threads=$OMP_NUM_THREADS"

# NOTE: the NUMA / STREAM peak baseline (formerly T1 here) is now owned
# by E0_NUMA; run `sbatch ../E0_NUMA/run_beverin.sh` separately.

# --- T2 build (flags from common/setup_beverin.sh) -----------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o addusxx_cpu main_cpu.cpp ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -x hip -fgpu-rdc -o addusxx_gpu main_hip.cpp ${GPU_LDFLAGS}

# --- run (binaries write CSV at the path passed as argv[1]) --------------
./addusxx_cpu results/beverin/addusxx_cpu_sweep.csv
./addusxx_gpu results/beverin/addusxx_gpu_sweep.csv

echo "[E5 beverin] done. CSVs under results/beverin/"
