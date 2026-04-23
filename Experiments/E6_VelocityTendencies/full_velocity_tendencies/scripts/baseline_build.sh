#!/usr/bin/env bash
#
# Build the numerically-validating CPU baseline that compiles the DaCe
# code-generated velocity_tendencies.cpp against a hand-written main.
# Used to cross-check DaCe SDFG output against the Fortran reference.
#
# Assumes that the CPU environment (compiler, OpenMP, libnuma) has
# already been set up via ../../../common/activate.sh plus
# ../../../common/setup_{daint,beverin}.sh. DACE_INCDIR is derived
# from the active DaCe install; DaCe must be importable.

set +x

: "${DACE_INCDIR:=$(python -c 'import dace, os; print(os.path.join(os.path.dirname(dace.__file__), "runtime", "include"))')}"
: "${VELOCITY_INCDIR:=generated_data/codegen/cpu/include}"
: "${SERDE_INCDIR:=generated_data}"
: "${VELOCITY_CC:=generated_data/codegen/cpu/src/cpu/velocity_tendencies.cpp}"
: "${CXX:=c++}"

export ASAN_OPTIONS=new_delete_type_mismatch=0

${CXX} ${VELOCITY_CC} main.cc -o numerically_validate \
  -I"${DACE_INCDIR}" \
  -I"${VELOCITY_INCDIR}" \
  -I"${SERDE_INCDIR}" \
  -Wno-parentheses-equality -std=c++20 -fPIC -fsanitize=address -fsanitize=undefined -g -O0
