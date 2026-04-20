#!/usr/bin/env bash
#SBATCH --job-name=E2_conj_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=03:30:00
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=192
#SBATCH --exclusive
#SBATCH --output=results/beverin/E2_conj_beverin_%j.out
#SBATCH --error=results/beverin/E2_conj_beverin_%j.err
#
# E2 Complex Conjugation (Figure 8) on Beverin.

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"
cd "${EXP_DIR}"

echo "[E2 beverin] host=$(hostname) threads=$OMP_NUM_THREADS"

# --- build (flags from common/setup_beverin.sh) --------------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o conjugate_cpu_inplace conjugate_inplace.cpp     ${CPU_LDFLAGS}
${CPU_CXX} ${CPU_CXXFLAGS} -o conjugate_cpu_oop     conjugate.cpp             ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o conjugate_gpu_inplace conjugate_inplace_hip.cpp ${GPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o conjugate_gpu_oop     conjugate_hip.cpp         ${GPU_LDFLAGS}

# --- run (binaries write CSV at the path passed as argv[1]) --------------
./conjugate_cpu_inplace results/beverin/results_cpu_inplace.csv
./conjugate_cpu_oop     results/beverin/results_cpu_oop.csv
./conjugate_gpu_inplace results/beverin/results_gpu_inplace.csv
./conjugate_gpu_oop     results/beverin/results_gpu_oop.csv

echo "[E2 beverin] done. CSVs under results/beverin/"
