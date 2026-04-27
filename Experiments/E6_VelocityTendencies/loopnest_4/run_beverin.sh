#!/usr/bin/env bash
#SBATCH --job-name=E6L4_ddtvnvert_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=24:00:00
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=192
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/beverin/E6L4_ddtvnvert_beverin_%j.out
#SBATCH --error=results/beverin/E6L4_ddtvnvert_beverin_%j.err
#
# E6 / loopnest_4 -- indirect stencil + CFL-gated diffusion (partial vertical)
# sweep on Beverin.

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"
cd "${EXP_DIR}"

export ICON_DATA_PATH="${ICON_DATA_PATH:-${EXP_DIR}/data_r02b05}"

echo "[E6L4 beverin] host=$(hostname) threads=$OMP_NUM_THREADS data=$ICON_DATA_PATH"

# NOTE: the NUMA / STREAM peak baseline (formerly T1 here) is now owned
# by E0_NUMA; run `sbatch ../../E0_NUMA/run_{daint,beverin}.sh` separately.

${CPU_CXX} ${CPU_CXXFLAGS}            -o bench_cpu_a  bench_cpu.cpp     ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -fgpu-rdc  -o bench_gpu_a  -x hip bench_gpu.cu ${GPU_LDFLAGS}

./bench_cpu_a results/beverin/ddt_vn_vert_cpu.csv
./bench_gpu_a results/beverin/ddt_vn_vert_gpu.csv

${CPU_CXX} ${CPU_CXXFLAGS} -o cost_metrics cost_metrics.cpp ${CPU_LDFLAGS}
./cost_metrics results/beverin/metrics_cpu_nl90.csv  81920 90 1 0.012 1.8 4 32768
./cost_metrics results/beverin/metrics_gpu_nl90.csv  81920 90 1 0.012 1.8 4 16384

echo "[E6L4 beverin] done. CSVs under results/beverin/"
