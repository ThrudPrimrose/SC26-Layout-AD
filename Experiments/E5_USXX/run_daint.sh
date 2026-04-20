#!/usr/bin/env bash
#SBATCH --job-name=E5_addusxx_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=04:00:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=288
#SBATCH --exclusive
#SBATCH --output=results/daint/E5_addusxx_daint_%j.out
#SBATCH --error=results/daint/E5_addusxx_daint_%j.err
#
# E5 USXX (addusxx_g) AoS vs SoA sweep on Daint.Alps.

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"
cd "${EXP_DIR}"

[[ -d bin ]] || bash download_data.sh

echo "[E5 daint] host=$(hostname) threads=$OMP_NUM_THREADS"

# --- T1 STREAM peak (scale-add RMW, huge square matrix, NUMA + hugepages) -
${CPU_CXX} ${CPU_CXXFLAGS} -o bench_stream_cpu "${COMMON_DIR}/bench_stream.cpp"        ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o bench_stream_gpu "${COMMON_DIR}/bench_stream_gpu.cu"     ${GPU_LDFLAGS}
./bench_stream_cpu results/daint/stream_peak_cpu.csv
./bench_stream_gpu results/daint/stream_peak_gpu.csv

# --- T2 build (flags from common/setup_daint.sh) -------------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o addusxx_cpu main_cpu.cpp ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o addusxx_gpu main.cu      ${GPU_LDFLAGS}

# --- run (binaries write CSV at the path passed as argv[1]) --------------
./addusxx_cpu results/daint/addusxx_cpu_sweep.csv
./addusxx_gpu results/daint/addusxx_gpu_sweep.csv

echo "[E5 daint] done. CSVs under results/daint/"
