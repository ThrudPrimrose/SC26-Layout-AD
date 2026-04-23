#!/usr/bin/env bash
# Beverin (MI300A) platform env. Sourced after common/activate.sh.
# Loads ROCm + GCC via spack; Python comes from the spack-python venv
# activated by ../common/activate.sh (manual VIRTUAL_ENV + PATH export).

spack load gcc/ktd4slj  # 14.x for HIP host compiler

export ROCM_HOME=/opt/rocm
export HIP_PATH=$ROCM_HOME
export HIPCC=$ROCM_HOME/bin/hipcc
export PATH=$ROCM_HOME/bin:$PATH
export LD_LIBRARY_PATH=$ROCM_HOME/lib:$ROCM_HOME/lib64:${LD_LIBRARY_PATH:-}
export CPATH=$ROCM_HOME/include:${CPATH:-}
export LIBRARY_PATH=$ROCM_HOME/lib:$ROCM_HOME/lib64:${LIBRARY_PATH:-}
export CFLAGS="-I$ROCM_HOME/include ${CFLAGS:-}"
export LDFLAGS="-L$ROCM_HOME/lib -L$ROCM_HOME/lib64 ${LDFLAGS:-}"

export HCC_AMDGPU_TARGET=gfx942
export CUPY_INSTALL_USE_HIP=1
export CUPY_HIPCC_GENERATE_CODE=--offload-arch=gfx942
export ARCH=gfx942

export __HIP_PLATFORM_AMD__=1
export HIP_PLATFORM_AMD=1
export _DACE_NO_SYNC=1
export BEVERIN=1

# --- OpenMP / SLURM pinning (Zen4: 4 NUMA × 24 cores) --------------------
export OMP_NUM_THREADS=96
export OMP_PROC_BIND=close
export OMP_PLACES="{0}:24:1,{24}:24:1,{48}:24:1,{72}:24:1"
export OMP_SCHEDULE=static
export OMP_DISPLAY_ENV=TRUE
export SLURM_CPU_BIND=cores

# --- build flags ---------------------------------------------------------
export CPU_CXX="${CPU_CXX:-g++}"
export CPU_CXXFLAGS="${CPU_CXXFLAGS:--O3 -march=native -mtune=native -fopenmp -ffast-math -fno-vect-cost-model -std=c++17}"
export CPU_LDFLAGS="${CPU_LDFLAGS:--lnuma}"

export GPU_CXX="${GPU_CXX:-hipcc}"
export GPU_CXXFLAGS="${GPU_CXXFLAGS:--O3 -std=c++17 --offload-arch=${ARCH} -march=native -mtune=native -ffast-math -munsafe-fp-atomics -mllvm -amdgpu-early-inline-all=true -mllvm -amdgpu-function-calls=false -fgpu-flush-denormals-to-zero -D__HIP_PLATFORM_AMD__=1 -DHIP_PLATFORM_AMD=1 -fopenmp=libgomp}"
export GPU_LDFLAGS="${GPU_LDFLAGS:--lnuma}"

# Optional: OpenBLAS for CPU baselines (E3 transpose uses this).
if spack find -l openblas@0.3.30 >/dev/null 2>&1; then
  export OPENBLAS_HOME=$(spack location -i openblas@0.3.30)
  export C_INCLUDE_PATH=$OPENBLAS_HOME/include:${C_INCLUDE_PATH:-}
  export CPLUS_INCLUDE_PATH=$OPENBLAS_HOME/include:${CPLUS_INCLUDE_PATH:-}
  export LIBRARY_PATH=$OPENBLAS_HOME/lib:$OPENBLAS_HOME/lib64:${LIBRARY_PATH:-}
  export LD_LIBRARY_PATH=$OPENBLAS_HOME/lib:$OPENBLAS_HOME/lib64:${LD_LIBRARY_PATH:-}
  export PATH=$OPENBLAS_HOME/bin:${PATH}
fi

# Make the spack-python headers visible to CuPy / JIT compilation.
_PY_INC="$(python -c 'import sysconfig; print(sysconfig.get_path("include"))')"
export CFLAGS="-I${_PY_INC} ${CFLAGS}"
export C_INCLUDE_PATH="${_PY_INC}:${C_INCLUDE_PATH:-}"

if [[ -n "${SCRATCH:-}" ]]; then
  export C_INCLUDE_PATH=$SCRATCH/include:$C_INCLUDE_PATH
  export CPLUS_INCLUDE_PATH=$SCRATCH/include:${CPLUS_INCLUDE_PATH:-}
  export LIBRARY_PATH=$SCRATCH/lib:$SCRATCH/lib64:$LIBRARY_PATH
  export LD_LIBRARY_PATH=$SCRATCH/lib:$SCRATCH/lib64:$LD_LIBRARY_PATH
  export PATH=$SCRATCH/bin:$PATH
fi
