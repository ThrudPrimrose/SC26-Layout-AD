#!/usr/bin/env bash
#SBATCH --job-name=E2_conj_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=03:30:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=288
#SBATCH --output=results/daint/E2_conj_daint_%j.out
#SBATCH --error=results/daint/E2_conj_daint_%j.err
#
# E2 Complex Conjugation (Figure 8) on Daint.Alps.

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"
cd "${EXP_DIR}"

echo "[E2 daint] host=$(hostname) threads=$OMP_NUM_THREADS"

# --- build (flags from common/setup_daint.sh) ----------------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o conjugate_cpu_inplace conjugate_inplace.cpp ${CPU_LDFLAGS}
${CPU_CXX} ${CPU_CXXFLAGS} -o conjugate_cpu_oop     conjugate.cpp          ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o conjugate_gpu_inplace conjugate_inplace.cu   ${GPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o conjugate_gpu_oop     conjugate.cu           ${GPU_LDFLAGS}

# --- run (binaries write CSV at the path passed as argv[1]) --------------
./conjugate_cpu_inplace results/daint/results_cpu_inplace.csv
./conjugate_cpu_oop     results/daint/results_cpu_oop.csv
./conjugate_gpu_inplace results/daint/results_gpu_inplace.csv
./conjugate_gpu_oop     results/daint/results_gpu_oop.csv

echo "[E2 daint] done. CSVs under results/daint/"
