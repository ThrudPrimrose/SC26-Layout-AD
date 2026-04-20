#!/usr/bin/env bash
#SBATCH --job-name=E3_transpose_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=04:00:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=288
#SBATCH --exclusive
#SBATCH --output=results/daint/E3_transpose_daint_%j.out
#SBATCH --error=results/daint/E3_transpose_daint_%j.err
#
# E3 Matrix Transpose (Figure 9, Table III) on Daint.Alps.

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"
cd "${EXP_DIR}"
export RESULTS_DIR="${EXP_DIR}/results/daint"

echo "[E3 daint] host=$(hostname) threads=$OMP_NUM_THREADS"

# --- T1 STREAM peak (scale-add RMW, huge square matrix, NUMA + hugepages) -
${CPU_CXX} ${CPU_CXXFLAGS} -o bench_stream_cpu "${COMMON_DIR}/bench_stream.cpp"        ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o bench_stream_gpu "${COMMON_DIR}/bench_stream_gpu.cu"     ${GPU_LDFLAGS}
./bench_stream_cpu "${RESULTS_DIR}/stream_peak_cpu.csv"
./bench_stream_gpu "${RESULTS_DIR}/stream_peak_gpu.csv"

# --- T2 build + sweep (Python drivers honor RESULTS_DIR) -----------------
python run_cpu_transpose.py --compile          # CPU kernels + HPTT + OpenBLAS
python run_transpose.py     --compile          # GPU kernels + cuTENSOR

# --- T3 cost metrics (CPU: 64 B line / 16 fp32; GPU: 128 B / 32 fp32) ----
${CPU_CXX} ${CPU_CXXFLAGS} -o cost_metrics cost_metrics.cpp ${CPU_LDFLAGS}
./cost_metrics 16384 16 8,16,32,64,128 "${RESULTS_DIR}/transpose_metrics_cpu.csv"
./cost_metrics 16384 32 8,16,32,64,128 "${RESULTS_DIR}/transpose_metrics_gpu.csv"

echo "[E3 daint] done. CSVs under results/daint/"
