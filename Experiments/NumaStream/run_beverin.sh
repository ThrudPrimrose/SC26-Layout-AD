#!/usr/bin/env bash
#SBATCH --job-name=SUP_numa_beverin
#SBATCH --nodes=1
#SBATCH --partition=mi300
#SBATCH --time=02:00:00
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=192
#SBATCH --exclusive
#SBATCH --chdir=.
#SBATCH --output=results/beverin/SUP_numa_beverin_%j.out
#SBATCH --error=results/beverin/SUP_numa_beverin_%j.err
#
# NumaStream (supplemental): C = alpha * (A + B) on very large fp64
# matrices. Beverin (MI300A APU) edition.
#
# Env overrides: NLIST, REPS, ALPHA. See run_daint.sh for documentation.

EXP_DIR="${SLURM_SUBMIT_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
COMMON_DIR="$(cd "${EXP_DIR}/../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_beverin.sh"

mkdir -p "${EXP_DIR}/results/beverin"
cd "${EXP_DIR}"

NLIST="${NLIST:-8192,16384,24576}"
WARMUP="${WARMUP:-5}"
NRUNS="${NRUNS:-100}"
ALPHA="${ALPHA:-1.0001}"

echo "[SUP numa beverin] host=$(hostname) threads=$OMP_NUM_THREADS"
echo "                   N=$NLIST warmup=$WARMUP nruns=$NRUNS alpha=$ALPHA"

${CPU_CXX} ${CPU_CXXFLAGS}           -o bench_cpu bench_cpu.cpp     ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -fgpu-rdc -o bench_gpu bench_gpu_hip.cpp ${GPU_LDFLAGS}

./bench_cpu results/beverin/numa_cpu.csv "${NLIST}" "${WARMUP}" "${NRUNS}" "${ALPHA}"
./bench_gpu results/beverin/numa_gpu.csv "${NLIST}" "${WARMUP}" "${NRUNS}" "${ALPHA}"

echo "[SUP numa beverin] done. CSVs under results/beverin/"
