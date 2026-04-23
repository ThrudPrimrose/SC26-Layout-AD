#!/usr/bin/env bash
#SBATCH --job-name=SUP_numa_daint
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --time=01:30:00
#SBATCH --account=g177-1
#SBATCH --ntasks=1
#SBATCH --gpus-per-task=1
#SBATCH --cpus-per-task=288
#SBATCH --exclusive
#SBATCH --output=results/daint/SUP_numa_daint_%j.out
#SBATCH --error=results/daint/SUP_numa_daint_%j.err
#
# Supplemental / NumaStream: C = alpha * (A + B) on very large fp64
# matrices. CPU bench sweeps {baseline_ft, numa4_stripe, interleave}
# across three matrix sizes; GPU bench sweeps 5 block sizes x 5 grid
# multipliers across the same three sizes. Daint.Alps edition (Grace +
# H200).
#
# Environment overrides honored by the benchmarks:
#   NLIST  : comma-separated list of N values (default "8192,16384,24576")
#   REPS   : timed iterations per variant (default 50)
#   ALPHA  : scalar constant in C = alpha * (A + B) (default 1.0001)

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMMON_DIR="$(cd "${EXP_DIR}/../../common" && pwd)"

source "${COMMON_DIR}/activate.sh"
source "${COMMON_DIR}/setup_daint.sh"

mkdir -p "${EXP_DIR}/results/daint"
cd "${EXP_DIR}"

NLIST="${NLIST:-8192,16384,24576}"
WARMUP="${WARMUP:-5}"
NRUNS="${NRUNS:-100}"
ALPHA="${ALPHA:-1.0001}"

echo "[SUP numa daint] host=$(hostname) threads=$OMP_NUM_THREADS"
echo "                 N=$NLIST warmup=$WARMUP nruns=$NRUNS alpha=$ALPHA"

${CPU_CXX} ${CPU_CXXFLAGS} -o bench_cpu bench_cpu.cpp ${CPU_LDFLAGS}
${GPU_CXX} ${GPU_CXXFLAGS} -o bench_gpu bench_gpu.cu  ${GPU_LDFLAGS}

./bench_cpu results/daint/numa_cpu.csv "${NLIST}" "${WARMUP}" "${NRUNS}" "${ALPHA}"
./bench_gpu results/daint/numa_gpu.csv "${NLIST}" "${WARMUP}" "${NRUNS}" "${ALPHA}"

echo "[SUP numa daint] done. CSVs under results/daint/"
