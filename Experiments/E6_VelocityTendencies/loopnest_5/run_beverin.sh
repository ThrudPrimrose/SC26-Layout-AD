#!/usr/bin/env bash
#SBATCH --job-name=E6L5_vnieboundary_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=04:00:00
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=192
#SBATCH --exclusive
#SBATCH --output=results/beverin/E6L5_vnieboundary_beverin_%j.out
#SBATCH --error=results/beverin/E6L5_vnieboundary_beverin_%j.err
#
# E6 / loopnest_5 -- horizontal-only boundary kernel (vn_ie top/bottom).
# sweep on Beverin.

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMMON_DIR="$(cd "${EXP_DIR}/../../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"
cd "${EXP_DIR}"

export ICON_DATA_PATH="${ICON_DATA_PATH:-/capstor/scratch/cscs/ybudanaz/beverin/icon-artifacts/velocity/data_r02b05}"

echo "[E6L5 beverin] host=$(hostname) threads=$OMP_NUM_THREADS data=$ICON_DATA_PATH"

${CPU_CXX} ${CPU_CXXFLAGS} -o bench_stream_cpu "${COMMON_DIR}/bench_stream.cpp"         ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o bench_stream_gpu "${COMMON_DIR}/bench_stream_gpu_hip.cpp" ${GPU_LDFLAGS}
./bench_stream_cpu results/beverin/stream_peak_cpu.csv
./bench_stream_gpu results/beverin/stream_peak_gpu.csv

${CPU_CXX} ${CPU_CXXFLAGS}            -o bench_cpu_a  bench_cpu.cpp     ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -fgpu-rdc  -o bench_gpu_a  bench_gpu_hip.cpp ${GPU_LDFLAGS}

./bench_cpu_a results/beverin/vn_ie_boundary_cpu.csv
./bench_gpu_a results/beverin/vn_ie_boundary_gpu.csv

${CPU_CXX} ${CPU_CXXFLAGS} -o cost_metrics cost_metrics.cpp ${CPU_LDFLAGS}
./cost_metrics results/beverin/metrics_cpu_nl90.csv  81920 90 0.6 0.4 0.05 0.25 32768
./cost_metrics results/beverin/metrics_gpu_nl90.csv  81920 90 0.6 0.4 0.05 0.25 16384

echo "[E6L5 beverin] done. CSVs under results/beverin/"
