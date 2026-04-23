#!/usr/bin/env bash
#SBATCH --job-name=E0_stream_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=00:30:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=288
#SBATCH --chdir=.
#SBATCH --output=results/daint/E0_stream_daint_%j.out
#SBATCH --error=results/daint/E0_stream_daint_%j.err
#
# E0 NUMA-aware STREAM peak on Daint.Alps (Grace CPU + Hopper GPU).
#
# Under sbatch, $BASH_SOURCE resolves to /var/spool/slurmd/jobXXX/slurm_script
# (a copy), so we resolve EXP_DIR from SLURM_SUBMIT_DIR first and fall back
# to $BASH_SOURCE for direct `bash run_daint.sh` invocations.

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"
cd "${EXP_DIR}"

echo "[E0 daint] host=$(hostname) threads=$OMP_NUM_THREADS"

# --- build (flags from common/setup_daint.sh) ----------------------------
${CPU_CXX} ${CPU_CXXFLAGS} -o bench_stream_cpu bench_stream.cpp ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -I"${COMMON_DIR}" -o bench_stream_gpu bench_stream_gpu.cu ${GPU_LDFLAGS}

# --- run -----------------------------------------------------------------
./bench_stream_cpu "results/daint/stream_peak_cpu.csv"
./bench_stream_gpu "results/daint/stream_peak_gpu.csv"

echo "[E0 daint] done. CSVs under results/daint/"
