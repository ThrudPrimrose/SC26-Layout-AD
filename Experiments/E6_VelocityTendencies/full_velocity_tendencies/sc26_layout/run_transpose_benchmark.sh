#!/bin/bash
# Build and run the transpose benchmark
# Assumes CUTENSOR_ROOT is set, or cutensor is in system paths

CUTENSOR_INC=""
CUTENSOR_LIB="-lcutensor"

if [ -n "$CUTENSOR_ROOT" ]; then
    CUTENSOR_INC="-I${CUTENSOR_ROOT}/include"
    CUTENSOR_LIB="-L${CUTENSOR_ROOT}/lib -lcutensor"
fi

set -ex
nvcc -O3 --use_fast_math -std=c++17 -arch=sm_86 -Xcompiler=-ffast-math -Xcompiler=-fno-trapping-math -Xcompiler=-fno-math-errno -Xcompiler=-fno-vect-cost-model \
    transpose_bench.cu \
    ${CUTENSOR_INC} \
    ${CUTENSOR_LIB} \
    -o transpose_bench

./transpose_bench