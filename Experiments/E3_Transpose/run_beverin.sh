#!/usr/bin/env bash
#SBATCH --job-name=E3_transpose_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=05:00:00
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=192
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/beverin/E3_transpose_beverin_%j.out
#SBATCH --error=results/beverin/E3_transpose_beverin_%j.err
#
# E3 Matrix Transpose (Figure 9, Table III) on Beverin.

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"
cd "${EXP_DIR}"
export RESULTS_DIR="${EXP_DIR}/results/beverin"

echo "[E3 beverin] host=$(hostname) threads=$OMP_NUM_THREADS"

# --- T2 build + sweep (Python drivers honor RESULTS_DIR) -----------------
# NOTE: the NUMA / STREAM peak baseline (formerly T1 here) is now owned
# by E0_NUMA; run `sbatch ../E0_NUMA/run_beverin.sh` separately.
python run_cpu_transpose.py --compile          # CPU kernels + HPTT + OpenBLAS
python run_transpose.py     --compile          # GPU kernels + hipTensor

# --- T3 cost metrics (CPU: 64 B line / 16 fp32; GPU: 128 B / 32 fp32) ----
${CPU_CXX} ${CPU_CXXFLAGS} -o cost_metrics cost_metrics.cpp ${CPU_LDFLAGS}
./cost_metrics 16384 16 8,16,32,64,128 "${RESULTS_DIR}/transpose_metrics_cpu.csv"
./cost_metrics 16384 32 8,16,32,64,128 "${RESULTS_DIR}/transpose_metrics_gpu.csv"

echo "[E3 beverin] done. CSVs under results/beverin/"
