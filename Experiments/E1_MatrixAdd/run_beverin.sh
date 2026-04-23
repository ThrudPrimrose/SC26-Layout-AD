#!/usr/bin/env bash
#SBATCH --job-name=E1_madd_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=02:00:00
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=192
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/beverin/E1_madd_beverin_%j.out
#SBATCH --error=results/beverin/E1_madd_beverin_%j.err
#
# E1 Matrix Addition (Figure 4) on Beverin (Zen4 CPU + MI300A APU).

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"
cd "${EXP_DIR}"

echo "[E1 beverin] host=$(hostname) threads=$OMP_NUM_THREADS"

# --- build (flags from common/setup_beverin.sh) --------------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o bench_cpu bench_cpu.cpp ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o bench_gpu bench_gpu_hip.cpp ${GPU_LDFLAGS}

# --- run -----------------------------------------------------------------
./bench_cpu "results/beverin/madd_beverin_cpu.csv"
./bench_gpu "results/beverin/madd_beverin_gpu.csv"

echo "[E1 beverin] done. CSVs under results/beverin/"
