#!/usr/bin/env bash
#SBATCH --job-name=E6L2_zwconcorrme_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=04:30:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=288
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/daint/E6L2_zwconcorrme_daint_%j.out
#SBATCH --error=results/daint/E6L2_zwconcorrme_daint_%j.err
#
# E6 / loopnest_2 -- z_w_concorr_me direct stencil sweep on Daint.Alps.

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"
cd "${EXP_DIR}"

# Resolve data_r02b05/: prefer E8's copy, then E7's, else fetch.
bash "${EXP_DIR}/../loopnest_1/download_data.sh" "${EXP_DIR}/data_r02b05"

export ICON_DATA_PATH="${ICON_DATA_PATH:-${EXP_DIR}/data_r02b05}"

echo "[E6L2 daint] host=$(hostname) threads=$OMP_NUM_THREADS data=$ICON_DATA_PATH"

# NOTE: the NUMA / STREAM peak baseline (formerly T1 here) is now owned
# by E0_NUMA; run `sbatch ../../E0_NUMA/run_{daint,beverin}.sh` separately.

# --- T2 build ------------------------------------------------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o bench_cpu_a bench_cpu.cpp ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o bench_gpu_a bench_gpu.cu  ${GPU_LDFLAGS}

# --- T3 run --------------------------------------------------------------
./bench_cpu_a results/daint/z_w_concorr_me_cpu.csv
./bench_gpu_a results/daint/z_w_concorr_me_gpu.csv

# --- T4 cost metrics -----------------------------------------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o cost_metrics cost_metrics.cpp ${CPU_LDFLAGS}
./cost_metrics results/daint/metrics_cpu_nl90.csv  81920 90 0.6 0.4 0.05 0.25 32768
./cost_metrics results/daint/metrics_gpu_nl90.csv  81920 90 0.6 0.4 0.05 0.25 16384

echo "[E6L2 daint] done. CSVs under results/daint/"
