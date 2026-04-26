#!/usr/bin/env bash
# regenerate_baselines.sh -- F90 -> stage 5 driver for E7.
#
# DEFAULT FLOW (recommended): SKIP_F2DACE=1 and rely on the SHIPPED
# baseline/velocity_no_nproma.sdfgz. Phase 0 (the f2dace Fortran frontend
# on f2dace/staging) is UNSTABLE on the velocity AST -- it occasionally
# produces SDFGs that phase 1's StructToContainerGroups cannot flatten,
# raising rename-collision KeyErrors. The shipped post-phase-0 SDFG is the
# maintainer-validated input that phases 1..6 are tested against.
#
# Phases:
#   0  (DANGEROUS): velocity_modified.f90 -> baseline/velocity_no_nproma.sdfgz
#   1: generate_baselines.py: AoS -> SoA + symbol resolution + 4 specialised variants
#   2..6: utils.stages.stage{1..5} --optimize  -> codegen/stageN/<variant>.sdfgz
#
# Phase 0 needs DaCe on f2dace/staging; the rest run on yakup/dev. Branch
# switching is the caller's responsibility -- this script never mutates
# $DACE_DIR.
#
# Env overrides:
#   PYTHON              python interpreter (default: python)
#   SKIP_F2DACE         set to 1 to skip phase 0 (use the shipped
#                       baseline/velocity_no_nproma.sdfgz). RECOMMENDED.
#   ONLY_PHASE          run a single phase: 0..6 (default: all)
#   STAGE_FLAGS         extra args forwarded to each utils.stages.stage*
#                       invocation (default: --optimize)

set -euo pipefail

EXP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON="${PYTHON:-python}"
SKIP_F2DACE="${SKIP_F2DACE:-1}"   # default-skip phase 0; opt in by setting to 0
ONLY_PHASE="${ONLY_PHASE:-all}"
STAGE_FLAGS="${STAGE_FLAGS:---optimize}"

cd "${EXP_DIR}"

run_phase() {
  local phase="$1"
  if [[ "${ONLY_PHASE}" != "all" && "${ONLY_PHASE}" != "${phase}" ]]; then
    return 1
  fi
  return 0
}

if run_phase 0 && [[ "${SKIP_F2DACE}" != "1" ]]; then
  echo "[regenerate_baselines] phase 0: f2dace velocity_modified.f90 -> baseline/velocity_no_nproma.sdfgz"
  "${PYTHON}" tools/sdfg_from_velocity_f90.py \
      --input  baseline_inputs/velocity_modified.f90 \
      --output baseline/velocity_no_nproma.sdfgz
fi

if run_phase 1; then
  echo "[regenerate_baselines] phase 1: generate_baselines.py (AoS->SoA + 4 variants)"
  "${PYTHON}" generate_baselines.py \
      --input      baseline/velocity_no_nproma.sdfgz \
      --output-dir baseline \
      --data-dir   data_r02b05
fi

for STAGE in 1 2 3 4 5; do
  if run_phase "$((STAGE + 1))"; then
    echo "[regenerate_baselines] phase $((STAGE + 1)): utils.stages.stage${STAGE} ${STAGE_FLAGS}"
    "${PYTHON}" -m utils.stages.stage${STAGE} ${STAGE_FLAGS}
  fi
done

echo "[regenerate_baselines] done. Latest SDFGs under codegen/stage5/"
