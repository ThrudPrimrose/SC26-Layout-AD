#!/usr/bin/env bash
# Daint.Alps (GH200) platform env. Sourced after common/activate.sh.
# Loads GCC + CUDA + cuTENSOR via spack; Python comes from the spack-python
# venv activated by ../common/activate.sh (manual VIRTUAL_ENV + PATH export).
unset __HIP_PLATFORM_AMD__ HIP_PLATFORM_AMD BEVERIN

# Defensively scrub AMD-platform env vars in case the shell was previously
# polluted by a ``source setup_beverin.sh`` (or a cluster-site profile).
# ``utils/compile_if_propagated_sdfgs.py`` reads ``__HIP_PLATFORM_AMD__`` /
# ``HIP_PLATFORM_AMD`` ONCE at module import to decide whether to build
# with hipcc/HIP or nvcc/CUDA -- if either var is set when daint imports
# that module, daint will try to build with hipcc and fail with
# ``hipcc: command not found``.
unset __HIP_PLATFORM_AMD__ HIP_PLATFORM_AMD BEVERIN

spack load gcc/76jw6nu   # 14.3
spack load cuda@12.9
spack load cutensor

export CUDA_HOME=$(spack location -i cuda@12.9)
export CUTENSOR_HOME=$(spack location -i cutensor)

export PATH=$CUDA_HOME/bin:$CUTENSOR_HOME/bin:${PATH}
export LD_LIBRARY_PATH=$CUDA_HOME/lib64:$CUTENSOR_HOME/lib/12:${LD_LIBRARY_PATH:-}
export LIBRARY_PATH=$CUDA_HOME/lib64:$CUTENSOR_HOME/lib/12:${LIBRARY_PATH:-}
export C_INCLUDE_PATH=$CUDA_HOME/include:$CUTENSOR_HOME/include:${C_INCLUDE_PATH:-}
export CPLUS_INCLUDE_PATH=$CUDA_HOME/include:$CUTENSOR_HOME/include:${CPLUS_INCLUDE_PATH:-}

export ARCH=sm_90
export DAINT=1

# Stage 8 / propagated-SDFG compile honors _RELEASE: '1' picks the
# release flag set in utils/stages/common.py:compile_action() (-O3
# --use_fast_math, no -g). Default '0' (debug) is intended for codegen
# triage and produces unusable run-time numbers, so pin release here so
# every E8 sbatch defaults to release builds. Override with
# ``_RELEASE=0 sbatch ...`` to debug.
export _RELEASE="${_RELEASE:-1}"

# Daint special case: alias `python` to system python3.11 outside the venv.
# Inside the venv, ${VIRTUAL_ENV}/bin/python wins via PATH precedence and
# this alias is shadowed.
alias python=/usr/bin/python3.11

# --- OpenMP / SLURM pinning (Grace: 4 NUMA × 72 cores) -------------------
export OMP_NUM_THREADS=288
export OMP_PROC_BIND=close
export OMP_PLACES="{0}:72:1,{72}:72:1,{144}:72:1,{216}:72:1"
export OMP_SCHEDULE=static
export OMP_DISPLAY_ENV=TRUE
export SLURM_CPU_BIND=cores

# --- build flags ---------------------------------------------------------
export CPU_CXX="${CPU_CXX:-g++}"
export CPU_CXXFLAGS="${CPU_CXXFLAGS:--O3 -march=native -mtune=native -fopenmp -ffast-math -fno-trapping-math -fno-math-errno -fno-vect-cost-model -std=c++17}"
export CPU_LDFLAGS="${CPU_LDFLAGS:--lnuma}"

export GPU_CXX="${GPU_CXX:-nvcc}"
export GPU_CXXFLAGS="${GPU_CXXFLAGS:--O3 --use_fast_math -arch=${ARCH} -std=c++17 -Xcompiler=-fopenmp -Xcompiler=-ffast-math -Xcompiler=-fno-trapping-math -Xcompiler=-fno-math-errno -Xcompiler=-fno-vect-cost-model -Xcompiler=-march=native -Xcompiler=-mtune=native}"
export GPU_LDFLAGS="${GPU_LDFLAGS:--lnuma}"

# Optional: OpenBLAS for CPU baselines (E3).
if spack find -l openblas@0.3.29 >/dev/null 2>&1; then
  export OPENBLAS_HOME=$(spack location -i openblas@0.3.29)
  export C_INCLUDE_PATH=$OPENBLAS_HOME/include:$C_INCLUDE_PATH
  export CPLUS_INCLUDE_PATH=$OPENBLAS_HOME/include:$CPLUS_INCLUDE_PATH
  export LIBRARY_PATH=$OPENBLAS_HOME/lib:$OPENBLAS_HOME/lib64:$LIBRARY_PATH
  export LD_LIBRARY_PATH=$OPENBLAS_HOME/lib:$OPENBLAS_HOME/lib64:$LD_LIBRARY_PATH
fi

if [[ -n "${SCRATCH:-}" ]]; then
  export C_INCLUDE_PATH=$SCRATCH/include:$C_INCLUDE_PATH
  export CPLUS_INCLUDE_PATH=$SCRATCH/include:$CPLUS_INCLUDE_PATH
  export LIBRARY_PATH=$SCRATCH/lib:$SCRATCH/lib64:$LIBRARY_PATH
  export LD_LIBRARY_PATH=$SCRATCH/lib:$SCRATCH/lib64:$LD_LIBRARY_PATH
  export PATH=$SCRATCH/bin:$PATH
fi

# --- Auto-fetch experiment data if missing -------------------------------
# Each download_*.sh is internally idempotent (skips when target is
# already populated). EXP_DIR is set by the caller (run_<exp>_daint.sh)
# before sourcing this script.
if [[ -n "${EXP_DIR:-}" ]]; then
  shopt -s nullglob
  for _dl in "${EXP_DIR}/download_data.sh" "${EXP_DIR}"/scripts/download_*_data.sh; do
    [[ -f "${_dl}" ]] || continue
    echo "[setup_daint] data check via $(basename "${_dl}")"
    ( cd "$(dirname "${_dl}")" && bash "$(basename "${_dl}")" )
  done
  shopt -u nullglob
  unset _dl
fi
