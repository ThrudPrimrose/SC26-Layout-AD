# E7 — Full velocity tendencies (stage 5a permutation / stage 5b compression)

GPU layout-permutation sweep on the full ICON velocity-tendencies
module. E7 is a frozen snapshot of the
[VelocityTendenciesPipeline](../../../VelocityTendenciesPipeline) tree
plus AD glue (data downloader, optional F90 → stage 5 regenerator,
config inspector, static `run_{daint,beverin}.sh`).

## Prerequisites

E7 reads `../E6_VelocityTendencies/access_analysis/layout_candidates.json`
to derive its layout configs. **Run E6 task `T_{6.2}` first**, or rely
on the snapshot already committed under E6.

## Default flow

**One command per platform** — `run_{daint,beverin}.sh` is the
single entry point. It auto-fetches data, symlinks the shipped
stage 4 SDFGs (`SDFGs/stage4/` → `codegen/stage4/`), then runs
`stage5a` (the permutation sweep) for each layout config. No
manual stage1–4 invocation is needed.

```bash
cd Experiments/E7_FullVelocityTendencies
sbatch run_daint.sh        # NVIDIA H200 / GH200
sbatch run_beverin.sh      # AMD MI300A
```

The R02B05 / nproma=20480 dataset (~9 GB) is auto-fetched on first
submission via `tools/download_data.sh` (matches E4/E5/E6/loopnest_1's
guard pattern: `[[ -d data_r02b05 ]] && [[ -n "$(ls -A ...)" ]] || bash
tools/download_data.sh`). To pre-fetch or symlink to an existing copy:

```bash
bash tools/download_data.sh                            # download from PolyBox
LOCAL_DATA_DIR=~/Work/icon-artifacts/velocity/data_r02b05 \
    bash tools/download_data.sh                        # symlink an existing copy
```

`run_{daint,beverin}.sh` follow the E1–E6 convention: SBATCH header,
source `../common/{activate,setup_<host>}.sh`, build via
`CPU_CXX{,FLAGS}` / `GPU_CXX{,FLAGS}`. Default config list is
`unpermuted nlev_first index_only`; override per submission:

```bash
CONFIGS="unpermuted curated_nlev_first" sbatch run_daint.sh
python tools/list_layout_configs.py            # see all available configs
```

## Optional: regenerate from F90

`tools/regenerate_baselines.sh` walks F90 → SDFG → stage 4. **Phase 0
(f2dace frontend) is marked dangerous**: the DaCe Fortran frontend on
`f2dace/staging` is unstable and the AoS→SoA rewrite that follows
(phase 1, `StructToContainerGroups`) regularly raises rename collisions
on the velocity AST. Reviewers should **use the shipped
`baseline/velocity_no_nproma.sdfgz`** and run with `SKIP_F2DACE=1`
instead of regenerating from F90.

The remaining phases (`yakup/dev`, the day-to-day branch) consume the
shipped baseline and produce `codegen/stage{1..4}/<variant>.sdfgz`.
Branch switching is the **caller's responsibility** — the script never
mutates `$DACE_DIR`.

```bash
# Default (recommended): skip phase 0, use the shipped baseline.
SKIP_F2DACE=1 bash tools/regenerate_baselines.sh

# Full flow (only if phase 0's f2dace frontend is known good on the
# checkout you have AND you have time to debug rename collisions):
( cd "$DACE_DIR" && git checkout f2dace/staging )
bash tools/regenerate_baselines.sh
( cd "$DACE_DIR" && git checkout yakup/dev )
```

| Phase | Output | Driver | Stability |
|-------|--------|--------|-----------|
| 0 ⚠️  | `baseline/velocity_no_nproma.sdfgz` | `python tools/sdfg_from_velocity_f90.py` | **dangerous** — needs `f2dace/staging`; unstable on the velocity AST |
| 1 | `baseline/..._{0,1}_istep_{1,2}.sdfgz` | `python generate_baselines.py` | brittle — `StructToContainerGroups` may raise rename collisions |
| 2..5 | `codegen/stage{1..4}/<variant>.sdfgz` | `python -m utils.stages.stage{1..4} --optimize` | stable on `yakup/dev` |
| 6 | `codegen/stage5a/<config>/<variant>.sdfgz` (or `stage5b/...`) | `python -m utils.stages.stage5a --optimize --config <name>` | stable on `yakup/dev` (auto-invoked by `run_*.sh`) |

Env: `PYTHON`, `SKIP_F2DACE` (recommended `=1`), `ONLY_PHASE`, `STAGE_FLAGS`.

## Stage 5a / 5b — split off stage 4

Stage 5 is split into two **mutually-exclusive** alternatives that
both consume `codegen/stage4/<variant>.sdfgz`. Pick whichever the
experiment needs:

### Stage 5a — permutation sweep

`utils/stages/stage5a.py` deepcopies each stage-4 SDFG and applies
one `PermuteConfig` at a time. Configs come from
`utils.passes.permute_configs.extended_configs_from_candidates`:

- **named** (heuristic): `unpermuted`, `nlev_first`, `index_only`.
- **listed** (per-nest): one `nest_<nid>_<shape>` per `all_nests[]` of
  shape `h` or `v.h` (the latter shortened to `vh`).
- **curated**: cleaned-up port of the icon-artifacts
  `sc26_layout/permute_stage4.py` 94-cell sweep
  (`cv*_ch*_f*_s*_n<perm>`) plus `curated_nlev_first` /
  `curated_index_only` with researcher-validated permutations (e.g.
  connectivity arrays use `[0, 2, 1]`, not the heuristic `[2, 0, 1]`).

All runnable via `python -m utils.stages.stage5a --config <name>`.
Output: `codegen/stage5a/<config>/<variant>.sdfgz`.

### Stage 5b — compression (+ always-on `levmask` transpose)

`utils/stages/stage5b.py` runs the three compression rewrites
(`fold_array_access_to_expression` for single-valued `*_blk`,
`generate_compressed_variant` with `uint16` for indices, `uint8` for
multi-valued `*_blk`) and then applies a forced
`levmask` / `gpu_levmask` transpose `[1, 0]` plus, optionally, any
per-array permutations from a JSON config:

```bash
python -m utils.stages.stage5b                       # compression + levmask only
python -m utils.stages.stage5b --config nlev_first   # also merge JSON permutations
```

Output: `codegen/stage5b/<config-or-default>/<variant>.sdfgz`.

### Force-permute overrides

`_FORCE_PERMUTED` in `utils/passes/permute_configs.py` lists arrays
permuted regardless of the chosen config (merged into every emitted
`PermuteConfig`, including `unpermuted`):

| Array | Permutation | Reason |
|-------|-------------|--------|
| `levmask` | `[1, 0]` | Fortran `levmask(nblks_c, nlev)` is column-major (jb contiguous); f2dace lowers to row-major (the opposite memory order), so `ANY(:, jk)` strides by `nlev`. Transpose restores Fortran-canonical layout and fixes the reduce stride. The heuristic mistypes `levmask` as 3D from the `v.h` nest it shares with `z_w_v` etc.; force overrides bypass that. |

## Layout

```
E7_FullVelocityTendencies/
├── README.md                                this file
├── run_{daint,beverin}.sh                   static SBATCH drivers
├── baseline_inputs/velocity_modified.f90    self-contained Fortran AST (only consumed by the dangerous phase 0)
├── baseline/velocity_no_nproma.sdfgz        SHIPPED post-phase-0 SDFG (committed; consumed by phase 1+)
├── baseline/                                + 4 specialised SDFG variants from phase 1   (gitignored)
├── SDFGs/stage4/                            shipped stage 4 SDFGs (default-flow input)
├── codegen/stage{1..4,5a,5b}/               per-stage outputs                      (gitignored)
├── data_r02b05/                             populated by tools/download_data.sh    (gitignored)
├── utils/, src/, include/, main.cpp,        snapshot of VelocityTendenciesPipeline
│   f90_to_sdfg.py, generate_baselines.py, tests/
├── tools/
│   ├── download_data.sh                     PolyBox nproma=20480 fetcher
│   ├── regenerate_baselines.sh              optional F90 → stage 4 driver
│   ├── sdfg_from_velocity_f90.py            f2dace stage 2
│   ├── analyze_lbounds.cpp                  used by generate_baselines.py
│   └── list_layout_configs.py               inspector: prints available configs
└── results/                                 per-platform CSVs                       (gitignored)
```

## Pass library

Passes used by stages 1–6. **Bold** = newly written for E7 / VTP.

| Pass | Purpose |
|------|---------|
| `compress_indices.py` | Stage 5: fold `*_blk` arrays to constant 1; emit uint16/uint8 compressed variants |
| `curated_configs.py` | **Static curated `PermuteConfig` set (94-cell sweep + curated nlev_first / index_only)** |
| `dealias_symbols.py` | Baseline: rename `tmp_struct_symbol_*` to mangled container.member names |
| `externalize_struct_accesses.py` | f2dace fix-up: lift inner struct accesses to top-level scalars |
| **`fuse_full_and_endpoint.py`** | **Fuse a full-range copy map with an adjacent endpoint-constant write into one map dispatched by `ConditionalBlock`** |
| `int64_to_int32.py` | Stage 3: demote 64-bit integer arrays / symbols to 32-bit |
| `offload_velocity_to_gpu.py` | Stage 4: GPU offload (ToGPU + KernelLaunchRestructure + persistent transients + body timer + async reductions) |
| `permute_configs.py` | Heuristic + per-nest config builders, **plus `_FORCE_PERMUTED` override layer** |
| `permute_layout.py` | Wraps `dace.transformation.layout.PermuteDimensions` with the `PermuteConfig` dataclass |
| `prepare_baseline.py` | Baseline: parse `sdfg.global_code` / `init_code`, register init-LHS as global symbols |
| `promote_maxvcfl.py` | Stage 1: promote scalar `maxvcfl` to a `[nproma, nlev]` transient |
| `remove_clip_count.py` | Stage 1: drop the `clip_count` cycle-counter that gates a `CYCLE` |
| `rename_stripped_symbols.py` | Baseline: rename `__f2dace_SA*` → `SA_*` so the framecode filter keeps them |
| `replace_reductions_with_tasklets.py` | Stage 4: emit `reduce_max_async_host_gpu(...)` tasklets for scalar-return reductions |
| `resolve_extent_offsets.py` | Baseline: fold `__f2dace_SOA*` symbols to 1 when `lbounds.csv` confirms |
| `resolve_extent_sizes.py` | Baseline: fold `__f2dace_SA*` symbols to scalars (`nproma`/`nblks_c`/...) |
| `set_transient_sdfg_lifetime.py` | Stage 3: every transient → `AllocationLifetime.SDFG` |
| `strip_s_suffix.py` | Baseline: drop the per-occurrence `_s_<idx>` segment |
| `unify_variant_signatures.py` | Baseline: pad signatures so all 4 variants share an arglist |
| `uniquify_difcoef.py` | Baseline: rename per-variant `difcoef` to disambiguate state labels |
| `baseline_fix.py` | Baseline: misc shim for stale f2dace artefacts |

### `fuse_full_and_endpoint` pattern

```
state A:                                  state B (immediate successor):
  Map[lev=0:N, *other]                      Map[*other]
    Tasklet(...) -> dst[*other, lev]          Tasklet(out=const) -> dst[*other, K_HI=N]
```

fuses to:

```
fused_state:
  Map[lev=0:N+1, *other]
    NestedSDFG with ConditionalBlock:
      lev < N : tasklet A
      else    : tasklet B
    -> dst[*other, lev]
```

State B is removed and the interstate edges are rewired (A's preds →
fused → B's succs). Tested by
[tests/passes/test_fuse_full_and_endpoint.py](tests/passes/test_fuse_full_and_endpoint.py)
(structural collapse, range union, body conditional shape, negative
case for misaligned endpoint).

## Tests

In-process, no GPU required:

```bash
PYTHONPATH=$DACE_DIR pytest tests/passes/
```

| Test | Kind | What it pins |
|------|------|--------------|
| `test_offload_velocity_to_gpu.py` | structural | Stage 4 schedule/storage assignment + ABI invariants |
| `test_permute_layout_copy_states.py` | structural | After `permute_layout`, stage-4 copy_in becomes the forward permute, copy_out the inverse; no extra `permute_after_*` states, no body double-write. Skips on DaCe forks without `permute_dimensions`. |
| `test_fuse_full_and_endpoint.py` | structural | Two adjacent (full, endpoint-zero) states collapse into one map + `ConditionalBlock`. |
| `test_permute_and_fuse_numerical.py` | **numerical** | Synthetic stage-4 SDFG (host → transient body → host), oracle baseline, then recompile after each of (permute only, fuse only, both). `array_equal` against the oracle — both passes must preserve bit-exact semantics. Pre-loads `libgomp.so.1`. |
| `tools/test_download_data.py` | shell | Subprocess-runs `tools/download_data.sh` against synthetic fixtures: 4 cases for `LOCAL_DATA_DIR` symlink mode + 1 idempotent-skip + 1 `URL=file://...` to exercise the curl/tar path offline. |

## Self-contained vs sibling-checkout

E7 carries its own copy of `utils/`, `generate_baselines.py`, etc., so
`python -m utils.stages.stage5a` / `stage5b` work without a sibling
[VelocityTendenciesPipeline](../../../VelocityTendenciesPipeline)
checkout. This is the AD freeze; the two trees will diverge over time
— upstream fixes must be re-applied to E7 manually.
