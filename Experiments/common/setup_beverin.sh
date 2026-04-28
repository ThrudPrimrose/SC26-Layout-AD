#!/usr/bin/env bash
# Beverin (MI300A) platform env. Sourced after common/activate.sh.
# Loads ROCm + GCC via spack; Python comes from the spack-python venv
# activated by ../common/activate.sh (manual VIRTUAL_ENV + PATH export).

spack load gcc/ktd4slj  # 14.x for HIP host compiler

export ROCM_HOME=/opt/rocm
# `ROCM_PATH` is the name clang++/hipcc actually look up (LLVM HIP toolchain
# code path). Mirror it from `ROCM_HOME` so both conventions are populated;
# the `--rocm-path` / `--hip-path` compile-time flags use the same value.
export ROCM_PATH=$ROCM_HOME
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

# AMD HIP backend lock-in (defense in depth -- any single layer can fail
# in a subprocess that strips env or under flaky --rocm-path probing):
#   HIP_PLATFORM=amd                     -- driver-level dispatch
#   __HIP_PLATFORM_AMD__=1 / -D macro    -- preprocessor for user code
#   HIP_PATH / ROCM_HOME / HIPCC env     -- where the toolchain lives
#   --rocm-path / --hip-path on cmd line -- pinned in the flags so a
#                                            subprocess that loses env
#                                            doesn't fall back to CUDA
#                                            (LLVM #63660; ROCm/HIP #1716)
#   --offload-arch=gfx942                -- AMDGPU target; would fail
#                                            loudly on the wrong backend
export HIP_PLATFORM=amd
export __HIP_PLATFORM_AMD__=1
export HIP_PLATFORM_AMD=1
export _DACE_NO_SYNC=1
export BEVERIN=1

# Stage 8 / propagated-SDFG compile honors _RELEASE (see setup_daint.sh).
export _RELEASE="${_RELEASE:-1}"

# --- libgomp shim (covers both stale + fresh E8 binaries) ---------------
# Symptom: ``libomp.so: cannot open shared object file: No such file or
# directory`` at runtime. Beverin doesn't ship LLVM's ``libomp``; clang/
# hipcc's default OpenMP runtime references ``libomp.so`` in DT_NEEDED
# unless the build pinned ``-fopenmp=libgomp`` end-to-end. The
# propagated-SDFG link step now does pin libgomp explicitly, but binaries
# from before that fix still fail to load.
#
# Belt-and-braces fix: create a writable shim dir with a
# ``libomp.so -> libgomp.so.1`` symlink and prepend it to
# ``LD_LIBRARY_PATH``. The dynamic loader then finds *something* named
# ``libomp.so`` (which is actually libgomp), GOMP_* symbols resolve, and
# every E8 binary -- old or new -- runs cleanly. Fresh binaries (which
# have ``libgomp.so.1`` in DT_NEEDED directly) are unaffected.
#
# Use the spack-loaded gcc's libgomp explicitly: ``which gcc`` on
# Beverin's login shells often returns ``/usr/bin/gcc`` (older system
# gcc) even after ``spack load``; the propagated-SDFG host compile uses
# the spack gcc, so we want the matching libgomp ABI. Resolve via
# ``spack location -i`` instead of relying on PATH. Falls back to
# whatever ``gcc`` is on PATH if spack isn't available.
#
# This is a runtime-only safety net; the proper compile/link fix in
# ``Experiments/E8_LegacyVT/utils/compile_if_propagated_sdfgs.py`` is
# still the canonical path and is what the AD ships.
_gcc_for_libgomp=""
if command -v spack >/dev/null 2>&1; then
    for _spec in gcc/ktd4slj gcc@14 gcc; do
        _prefix="$(spack location -i "${_spec}" 2>/dev/null)" || continue
        if [[ -x "${_prefix}/bin/gcc" ]]; then
            _gcc_for_libgomp="${_prefix}/bin/gcc"
            break
        fi
    done
fi
if [[ -z "${_gcc_for_libgomp}" ]]; then
    _gcc_for_libgomp="$(command -v gcc 2>/dev/null)"
fi
if [[ -n "${_gcc_for_libgomp}" ]]; then
    _gomp_path="$(${_gcc_for_libgomp} -print-file-name=libgomp.so.1 2>/dev/null)"
    if [[ -f "${_gomp_path}" ]]; then
        _shim_dir="${SCRATCH:-/tmp}/sc26_omp_shim"
        mkdir -p "${_shim_dir}"
        ln -sf "${_gomp_path}" "${_shim_dir}/libomp.so"
        export LD_LIBRARY_PATH="${_shim_dir}:${LD_LIBRARY_PATH:-}"
        # Also surface the gcc lib dir directly so binaries with
        # DT_NEEDED libgomp.so.1 (the new ones) find it without
        # relying on RUNPATH baked into the binary.
        export LD_LIBRARY_PATH="$(dirname ${_gomp_path}):${LD_LIBRARY_PATH}"
    fi
    unset _gomp_path _shim_dir
fi
unset _gcc_for_libgomp _prefix _spec

# --- OpenMP / SLURM pinning (Zen4: 4 NUMA × 24 cores) --------------------
export OMP_NUM_THREADS=96
export OMP_PROC_BIND=close
export OMP_PLACES="{0}:24:1,{24}:24:1,{48}:24:1,{72}:24:1"
export OMP_SCHEDULE=static
export OMP_DISPLAY_ENV=TRUE
export SLURM_CPU_BIND=cores

# --- build flags ---------------------------------------------------------
export CPU_CXX="${CPU_CXX:-g++}"
export CPU_CXXFLAGS="${CPU_CXXFLAGS:--O3 -march=native -mtune=native -fopenmp -ffast-math -fno-trapping-math -fno-math-errno -fno-vect-cost-model -std=c++17}"
export CPU_LDFLAGS="${CPU_LDFLAGS:--lnuma}"

export GPU_CXX="${GPU_CXX:-hipcc}"
export GPU_CXXFLAGS="${GPU_CXXFLAGS:--O3 -std=c++17 --offload-arch=${ARCH} --rocm-path=${ROCM_HOME} --hip-path=${ROCM_HOME} -march=native -mtune=native -ffast-math -fno-trapping-math -fno-math-errno -munsafe-fp-atomics -mllvm -amdgpu-early-inline-all=true -mllvm -amdgpu-function-calls=false -fgpu-flush-denormals-to-zero -D__HIP_PLATFORM_AMD__=1 -DHIP_PLATFORM_AMD=1 -fopenmp=libgomp}"
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

# --- AMD HIP backend sanity check ---------------------------------------
# Warn loudly if hipcc has been resolved to a CUDA-HIP install instead
# of a ROCm one; reviewers on mixed-SDK boxes have hit this in the wild.
if command -v hipconfig >/dev/null 2>&1; then
  _hip_plat="$(hipconfig --platform 2>/dev/null || echo unknown)"
  if [[ "${_hip_plat}" != "amd" ]]; then
    echo "[setup_beverin] WARNING: hipconfig --platform = '${_hip_plat}' (expected 'amd')." >&2
    echo "[setup_beverin]          Check that ROCm's hipcc is on PATH before any CUDA hipcc." >&2
  fi
  unset _hip_plat
fi

# --- Auto-fetch experiment data if missing -------------------------------
# Each download_*.sh is internally idempotent (skips when target is
# already populated). EXP_DIR is set by the caller (run_<exp>_beverin.sh)
# before sourcing this script.
if [[ -n "${EXP_DIR:-}" ]]; then
  shopt -s nullglob
  for _dl in "${EXP_DIR}/download_data.sh" "${EXP_DIR}"/scripts/download_*_data.sh; do
    [[ -f "${_dl}" ]] || continue
    echo "[setup_beverin] data check via $(basename "${_dl}")"
    ( cd "$(dirname "${_dl}")" && bash "$(basename "${_dl}")" )
  done
  shopt -u nullglob
  unset _dl
fi
