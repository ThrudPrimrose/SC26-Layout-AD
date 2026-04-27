#!/usr/bin/env bash
#SBATCH --job-name=E6L1_zvgradw_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=04:30:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=288
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/daint/E6L1_zvgradw_daint_%j.out
#SBATCH --error=results/daint/E6L1_zvgradw_daint_%j.err
#
# E6 / loopnest_1 -- z_v_grad_w indirect-stencil sweep on Daint.Alps.

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"
cd "${EXP_DIR}"

# R02B05 connectivity + serialised ICON fields. Override ICON_DATA_PATH
# in the environment for non-Daint locations.
export ICON_DATA_PATH="${ICON_DATA_PATH:-${EXP_DIR}/data_r02b05}"

echo "[E6L1 daint] host=$(hostname) threads=$OMP_NUM_THREADS data=$ICON_DATA_PATH"

# NOTE: the NUMA / STREAM peak baseline (formerly T1 here) is now owned
# by E0_NUMA; run `sbatch ../../E0_NUMA/run_{daint,beverin}.sh` separately.

# --- T2 build ------------------------------------------------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o bench_cpu_a            bench_cpu.cpp            ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o bench_gpu_a            bench_gpu.cu             ${GPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o bench_gpu_oldstyle     bench_gpu_oldstyle.cu    ${GPU_LDFLAGS}

# --- T3 run (binaries write CSV at the path passed as argv[1]) -----------
./bench_cpu_a        results/daint/z_v_grad_w_cpu.csv
./bench_gpu_a        results/daint/z_v_grad_w_gpu.csv
./bench_gpu_oldstyle results/daint/z_v_grad_w_gpu_old.csv

# --- T4 cost metrics (CPU: 64 B / 16 fp32; GPU: 128 B / 32 fp32) ---------
# argv: csv_path N nl beta alpha gamma p_numa l1_bytes
${CPU_CXX} ${CPU_CXXFLAGS} -o cost_metrics cost_metrics.cpp ${CPU_LDFLAGS}
./cost_metrics results/daint/metrics_cpu_nl90.csv  81920 90 0.6 0.4 0.05 0.25 32768
./cost_metrics results/daint/metrics_gpu_nl90.csv  81920 90 0.6 0.4 0.05 0.25 16384

echo "[E6L1 daint] done. CSVs under results/daint/"
