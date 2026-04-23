#!/usr/bin/env bash
#SBATCH --job-name=E0_stream_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=00:30:00
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=192
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/beverin/E0_stream_beverin_%j.out
#SBATCH --error=results/beverin/E0_stream_beverin_%j.err
#
# E0 NUMA-aware STREAM peak on Beverin (Zen4 CPU + MI300A APU).
#
# Under sbatch, $BASH_SOURCE resolves to /var/spool/slurmd/jobXXX/slurm_script
# (a copy), so we resolve EXP_DIR from SLURM_SUBMIT_DIR first and fall back
# to $BASH_SOURCE for direct `bash run_beverin.sh` invocations.

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"
cd "${EXP_DIR}"

echo "[E0 beverin] host=$(hostname) threads=$OMP_NUM_THREADS"

# --- build (flags from common/setup_beverin.sh) --------------------------
# hipcc accepts .cu files directly via `-x hip`, so there is no separate
# _hip.cpp shim — we feed the same bench_stream_gpu.cu to both platforms.
${CPU_CXX} ${CPU_CXXFLAGS} -o bench_stream_cpu bench_stream.cpp ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -x hip -I"${COMMON_DIR}" -o bench_stream_gpu bench_stream_gpu.cu ${GPU_LDFLAGS}

# --- run -----------------------------------------------------------------
./bench_stream_cpu "results/beverin/stream_peak_cpu.csv"
./bench_stream_gpu "results/beverin/stream_peak_gpu.csv"

echo "[E0 beverin] done. CSVs under results/beverin/"
