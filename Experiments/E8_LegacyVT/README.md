# E8 — Full velocity tendencies (legacy pipeline, AD default)

> **AD default for §IV-D / Figure 14 / Table V.** The sibling
> [`../E7_FullVelocityTendencies/`](../E7_FullVelocityTendencies/) is
> an in-progress SDFG-driven refactor and is not required for the AD.

GPU layout-permutation sweep on the full ICON velocity-tendencies
module, driving `icon-artifacts/sc26_layout` on DaCe `f2dace/staging`.
Reproduces the V_k winner comparison from paper Fig 13 / 14 / Tab V.

**Expected behaviour.** Vertical-first (V6) should match or beat both
the ICON-default unpermuted layout and V1/V2 on the dominant Config B
work mix; gains should be largest on MI300A (paper: ~1.17× MI300A,
~1.07× Hopper for Config B; ~1.06× / ~0.97× for Config A).

## Quick start

```bash
# 1. One-time setup (if not already done)
bash ../common/setup.sh

# 2. Submit (curated default: 67 configs / 134 binaries; see "Configs")
cd Experiments/E8_LegacyVT
sbatch run_daint.sh           # or run_beverin.sh

# 3. After the job finishes, plot
python plot_paper.py
```

The `run_{daint,beverin}.sh` scripts handle:

- DaCe branch switch to `f2dace/staging` (required for E8).
- Dataset auto-fetch (R02B05 / `nproma=20480` ICON; ~9 GB) — uses
  `data_r02b05/` if populated, else symlinks from E7's copy, else
  downloads via E7's `tools/download_data.sh`.
- Regeneration of E6's `layout_candidates.json` and `winners.json` if
  missing.
- Per-config compile + run via `run_stage8_permutations.py`.

## Configs

The default sweep (no `CONFIGS=` override) is **67 configs / 134
binaries**: 3 named (`nlev_first`, `index_only`, `winner_v1` =
unpermuted) + 64 unique `v123_*` cells from the per-group V_k
cross-product (`Experiments/E6_VelocityTendencies/full_velocity_tendencies/layout_crossproduct_v123.json`),
deduped by permute-map signature. The 64 = 2⁵ IC-axis × 2 IN-axis
(V6 collapses with V2 on the `n` group). Mirror of E7's bridge.

Named V$_k$ aliases (subset of the 67):

| Config | Mapping | Reproduces |
|---|---|---|
| `winner_v1` | identity (h_first + SoA-conn) | Fig 13 *Original Layout* |
| `winner_v2` | h_first + AoS-conn (`[0,2,1]` connectivity) | connectivity-only intermediate |
| `winner_v6` | v_first + AoS-conn (`[2,0,1]` connectivity, level-first data) | Fig 13 *Optimized Layout* / §IV-D adopted layout |

Override via the `CONFIGS` env (comma-separated). Common shortcuts:

```bash
# Minimal Fig 13 reproduction (V1 vs V6, ~1 hr)
CONFIGS="winner_v1,winner_v6" sbatch run_daint.sh

# Just the three named V_k winners (legacy behaviour)
CONFIGS="winner_v1,winner_v2,winner_v6" sbatch run_daint.sh

# Preview the full sweep without compiling
bash run_daint.sh --dry-run
python run_stage8_permutations.py --list   # all registered configs
```

The legacy 95-cell `cv*_ch*_f*_s*_n*` cross-product
(2×2×2×2×6−1 = 95) is still registered in
[`sc26_layout/permute_stage8.py`](sc26_layout/permute_stage8.py) and
selectable explicitly via `CONFIGS=cv0_ch0_f0_s0_n012,...`, but is no
longer the default.

## Outputs

Per-config TXTs (two per config — `_shuffled.txt`, `_unshuffled.txt`)
land under `{daint,beverin}_full_permutations_8/`, alongside the
Slurm `.out`/`.err`. Each TXT captures the binary's stdout — `[Timer]
Elapsed time: <ms>` lines + reduction-correctness asserts. The
plotters (`plot_paper.py`, `Figures/plot_paper_snapshot.sh`) parse
those timing lines.

## Dependencies

| Component | Source | Notes |
|---|---|---|
| DaCe | `f2dace/staging` | Required for E8's codegen pipeline. The run script switches to it automatically; revert to `yakup/dev` after the job (or use a separate clone). |
| AMD backend | `setup_beverin.sh` + `utils/compile_if_propagated_sdfgs.py` | On Beverin, sourcing `../common/setup_beverin.sh` exports `HIP_PLATFORM=amd`, mirrors `ROCM_HOME`/`ROCM_PATH`/`HIP_PATH`, and pins `--rocm-path` / `--hip-path` / `--offload-arch=gfx942` / `-fopenmp=libgomp` into `GPU_CXXFLAGS`. The propagated-SDFG compile path bakes the same `--rocm-path` / `--hip-path` / `-fopenmp=libgomp` pins into its per-translation-unit flags so a stripped-env subprocess can't fall back to a CUDA probe or to LLVM's `libomp` runtime. `hipconfig --platform` is checked at sourcing time and warns if hipcc is misresolved. |
| CUDA | **12.x** (12.9 verified) | CUDA 13 removes/relocates several CUB and runtime symbols DaCe relies on: `CTA_SYNC` (was a CUB macro for `__syncthreads()`), `cub::CountingInputIterator`, `cub::TransformInputIterator`, plus `cudaDeviceProp` field renames probed during init. The cluster pin (CUDA 12.9 via `Experiments/common/setup_daint.sh`) avoids all of them. A 6-line patch in DaCe's `runtime/include/dace/{cuda/multidim_gbar.cuh,reduction.h}` makes the build CUDA-13-clean if you need to upgrade later. |
| ICON dataset | R02B05 / `nproma=20480` (~9 GB) | Auto-fetched / symlinked to `data_r02b05/`. Same dataset as E7. |
| `layout_candidates.json` | `../E6_VelocityTendencies/access_analysis/select_loopnests.py` | Regenerated on first run if missing. |
| `winners.json` | `../E6_VelocityTendencies/access_analysis/derive_winners.py` | Empirical V$_k$ winners per loopnest, from E6's loopnest CSVs. Regenerated on first run if missing. |
| `layout_groups.json` | `../E6_VelocityTendencies/access_analysis/generate_layout_groups.py` | The 7 layout-equivalent groups + V$_k$ permutation table. Read by `permute_stage8.py`'s named-alias builders. |

## Pipeline internals (skim)

- [`sc26_layout/permute_stage8.py`](sc26_layout/permute_stage8.py) —
  `PERMUTE_CONFIGS` dict: named aliases (`nlev_first`, `index_only`,
  `winner_v{1,2,6}`), the legacy 95-cell `cv*_ch*_f*_s*_n*` sweep, and
  `permute_sdfg()` driver. The 64 v123_* cells are registered at
  import time by [`v123_bridge.py`](sc26_layout/v123_bridge.py) from
  E6's `layout_crossproduct_v123.json`; mirror of E7's
  `tools/run_layout_configs.py` semantics (axis projection: V1/V2 →
  h_first, V6 → v_first; V_odd → SoA, V_even → AoS).
- [`run_stage8_permutations.py`](run_stage8_permutations.py) — main
  loop. Picks the curated default when `--configs` is absent;
  supports `--list` / `--dry-run` / `--reps`. Compiles per-config
  binaries via `compile_gpu_stage8` and runs each twice
  (shuffled / unshuffled).
- [`tools/patch_codegen_reductions.py`](tools/patch_codegen_reductions.py)
  — post-codegen hook wired into `compile_action()` via
  `post_codegen_hook`. Patches every `src/cuda/*.cu` (NVIDIA),
  `src/cuda/hip/*.cpp` (AMD), and `src/cpu/*.cpp` (host glue) with:
  (1) `#include "reductions_device.cuh"` on device-scope files —
  defines `__REDUCE_DEVICE__`, which makes the reduction tasklet's
  `#ifdef` chain pick the `__device__`-callable branch
  (`reduce_*_device`) ahead of the host-side `reduce_*_gpu`. Required
  because the permute pass inlines reduction tasklets into `__device__
  loop_body_*` helpers; without it nvcc errors with "calling a
  __host__ function from a __device__ function". (2) `#include
  "reductions_kernel.cuh"` (host-side declarations) and, on CPU host
  glue, `#include "reductions_cpu.h"` (CPU-side `reduce_*_cpu`).
  (3) Every value-use of `__dace_current_stream` rewritten to
  `nullptr` (declarations and LHS-of-assignment kept intact). The
  upstream icon-artifacts contract is "consumer defines
  `__REDUCE_DEVICE__` via a global include"; DaCe's permute pass
  drops the include from `sdfg.global_code`, so the patcher restores
  it post-codegen. Idempotent.
- [`src/reductions_kernel.{cu,cpp}`](src/reductions_kernel.cu) — the
  host-side `reduce_*_gpu` impls. `.cu` is single-source
  CUDA + HIP via `#if __HIP_PLATFORM_AMD__`; `.cpp` is the HIP-only
  trim. Linked automatically by `compile_if_propagated_sdfgs` —
  `.cu` on NVIDIA, `.cpp` on AMD — alongside `src/reductions.cpp`
  (CPU `reduce_*_cpu`).
- [`utils/stages/compile_gpu_stage8.py`](utils/stages/compile_gpu_stage8.py)
  — produces the per-config
  `velocity_gpu.stage8_standalone_release_permuted_<cfg>_<ms|mu>`
  binaries.

## Statistical methodology

Same as the rest of the AD: 100 timed reps after 5 warm-ups, Jacobi
cache flush between reps (`common/jacobi_flush.h`), no outlier
trimming, Scott-rule violin KDE with 200 eval points.

## Failure modes

- **Binary aborts during load (`std::out_of_range` from `map::at`)**:
  the run scripts no longer use `set -e` / `pipefail`, so a single
  config crash logs a `[E8 <plat>] WARN: ... aborted (rc=N); continuing`
  line and moves on to the next config / timestep. Cores and got/want
  blobs are cleaned up automatically (unless `--verify` was passed).
- **Missing `f2dace/staging` branch**: the run script tries to switch
  branches; if you've checked DaCe out somewhere it can't `git
  checkout` cleanly, fall back to a fresh clone:
  ```bash
  git clone -b f2dace/staging https://github.com/spcl/dace.git \
      ${EXP_DIR}/../common/dace_f2dace
  DACE_DIR=${EXP_DIR}/../common/dace_f2dace sbatch run_daint.sh
  ```
- **`layout_candidates.json` regeneration fails (LOKI not installed)**:
  ship the JSON manually — the file is committed under
  `../E6_VelocityTendencies/access_analysis/`.
- **`error while loading shared libraries: libomp.so` (Beverin only)**:
  hipcc/clang's default OpenMP runtime is LLVM's `libomp`, which is
  not on Beverin's library path. Both `setup_beverin.sh`'s
  `GPU_CXXFLAGS` and `utils/compile_if_propagated_sdfgs.py` now pin
  `-fopenmp=libgomp` so the binary links GNU's `libgomp` instead. If
  you see this error after a custom build, check that the compile and
  link command lines both carry `-fopenmp=libgomp` (not bare
  `-fopenmp`).
- **`reduce_*_gpu undefined` / `__dace_current_stream undefined` /
  "calling a __host__ function from a __device__ function"**: these are
  all symptoms of a missing post-codegen reductions patch. The
  `post_codegen_hook` wired in `utils/stages/common.py` should fix
  them inline; if you see them, check that
  `tools/patch_codegen_reductions.py` is on disk + executable and that
  `compile_action()` is calling it. Re-run the patcher manually with
  `python tools/patch_codegen_reductions.py codegen` (idempotent).

## See also

- [`../E6_VelocityTendencies/`](../E6_VelocityTendencies/) — per-loopnest
  V$_k$ sweeps that feed E8's `winners.json`.
- [`../E6_VelocityTendencies/access_analysis/`](../E6_VelocityTendencies/access_analysis/)
  — the E6→E7/E8 audit chain (`layout_groups.json`, `winners.json`).
- [`../E7_FullVelocityTendencies/`](../E7_FullVelocityTendencies/) —
  WIP reimplementation; **skip for the AD**.
- [`ICONArtifact/`](ICONArtifact/) — vendored ICON source / data-loading
  helpers; *not* required for the default `sbatch run_*.sh` reproduction
  (the stage-8 SDFGs in `codegen/` are already lowered).
