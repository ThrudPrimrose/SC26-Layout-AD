#!/usr/bin/env bash
#SBATCH --job-name=E5_addusxx_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=05:00:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=288
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/daint/E5_addusxx_daint_%j.out
#SBATCH --error=results/daint/E5_addusxx_daint_%j.err
#
# E5 USXX (addusxx_g) AoS vs SoA sweep on Daint.Alps.

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"
cd "${EXP_DIR}"

[[ -d bin ]] || bash download_data.sh

echo "[E5 daint] host=$(hostname) threads=$OMP_NUM_THREADS"

# NOTE: the NUMA / STREAM peak baseline (formerly T1 here) is now owned
# by E0_NUMA; run `sbatch ../E0_NUMA/run_daint.sh` separately.

# --- T2 build (flags from common/setup_daint.sh) -------------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o addusxx_cpu main_cpu.cpp ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o addusxx_gpu main.cu      ${GPU_LDFLAGS}

# --- run (binaries write CSV at the path passed as argv[1]) --------------
./addusxx_cpu results/daint/addusxx_cpu_sweep.csv
./addusxx_gpu results/daint/addusxx_gpu_sweep.csv

echo "[E5 daint] done. CSVs under results/daint/"
