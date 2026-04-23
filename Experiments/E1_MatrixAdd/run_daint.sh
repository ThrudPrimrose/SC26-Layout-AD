#!/usr/bin/env bash
#SBATCH --job-name=E1_madd_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=02:00:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=288
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/daint/E1_madd_daint_%j.out
#SBATCH --error=results/daint/E1_madd_daint_%j.err
#
# E1 Matrix Addition (Figure 4) on Daint.Alps (Grace CPU + Hopper GPU).

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"
cd "${EXP_DIR}"

echo "[E1 daint] host=$(hostname) threads=$OMP_NUM_THREADS"

# --- build (flags from common/setup_daint.sh) ----------------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o bench_cpu bench_cpu.cpp ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o bench_gpu bench_gpu.cu ${GPU_LDFLAGS}

# --- run -----------------------------------------------------------------
./bench_cpu "results/daint/madd_daint_cpu.csv"
./bench_gpu "results/daint/madd_daint_gpu.csv"

echo "[E1 daint] done. CSVs under results/daint/"
