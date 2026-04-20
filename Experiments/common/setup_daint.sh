#!/usr/bin/env bash
# Daint.Alps (GH200) platform env. Sourced after common/activate.sh.
# Loads GCC + CUDA + cuTENSOR via spack; Python comes from the pyenv venv.

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

# --- OpenMP / SLURM pinning (Grace: 4 NUMA × 72 cores) -------------------
export OMP_NUM_THREADS=288
export OMP_PROC_BIND=close
export OMP_PLACES="{0}:72:1,{72}:72:1,{144}:72:1,{216}:72:1"
export OMP_DISPLAY_ENV=TRUE
export SLURM_CPU_BIND=cores

# --- build flags ---------------------------------------------------------
export CPU_CXX="${CPU_CXX:-g++}"
export CPU_CXXFLAGS="${CPU_CXXFLAGS:--O3 -march=native -mtune=native -fopenmp -ffast-math -fno-vect-cost-model -std=c++17}"
export CPU_LDFLAGS="${CPU_LDFLAGS:--lnuma}"

export GPU_CXX="${GPU_CXX:-nvcc}"
export GPU_CXXFLAGS="${GPU_CXXFLAGS:--O3 -arch=${ARCH} -std=c++17 -Xcompiler=-fopenmp}"
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
