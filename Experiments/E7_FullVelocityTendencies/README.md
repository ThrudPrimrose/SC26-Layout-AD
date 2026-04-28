# E7 — Full velocity tendencies (in-progress refactor)

> **Status: in-progress SDFG-driven refactor.** Not required for the
> AD reproduction path; the §IV-D / Figure 14 / Table V reproduction
> lives in [`../E8_LegacyVT/`](../E8_LegacyVT/). E7 rebuilds the same
> module on DaCe `yakup/dev` with `OffloadVelocityToGPU` +
> `PermuteDimensions`. Skip for the AD.
>
> **Do not run E7 and E8 concurrently against the same DaCe clone.**
> E7 pins `yakup/dev`, E8 pins `f2dace/staging`; they will fight over
> HEAD. Use a separate `$DACE_DIR` if you need both at once.

GPU layout-permutation sweep on the full ICON velocity-tendencies
module. E7 is a frozen snapshot of the
[VelocityTendenciesPipeline](../../../VelocityTendenciesPipeline) tree
plus AD glue (data downloader, optional F90 → stage 5 regenerator,
config inspector, static `run_{daint,beverin}.sh`).

## Prerequisites

E7 reads two artefacts from E6:

- `../E6_VelocityTendencies/access_analysis/layout_candidates.json`
  (run E6 task `T6.2`).
- `../E6_VelocityTendencies/full_velocity_tendencies/layout_crossproduct_winners.json`
  (run E6 task `T6.4` — `python generate_winners.py`). The sbatch
  driver will regenerate this for you on demand (see below), so you
  can skip the manual step if you trust the latest loopnest CSVs.

## Default flow

### Step 0 — fetch the dataset (do this once, before sbatch)

The R02B05 / `nproma=20480` ICON dataset is ~9 GB and has to live at
`data_r02b05/` before any binary can run. Fetch it explicitly so a
network glitch or `xz` PATH issue doesn't cost you GPU wallclock from
inside a Slurm job:

```bash
cd Experiments/E7_FullVelocityTendencies
bash tools/download_data.sh                            # PolyBox → data_r02b05/
# OR, if the data already lives on the cluster:
LOCAL_DATA_DIR=/path/to/existing/data_r02b05 bash tools/download_data.sh   # symlink, no copy
```

The script is idempotent — re-running it on a populated `data_r02b05/`
is a no-op. Override the URL with `URL=...` for mirror fallback,
verify the tarball with `EXPECTED_SHA256=...`, point at a different
xz binary with `XZ_BIN=/path/to/xz` if the cluster's spack-installed
xz has rotted. The sbatch driver also calls `download_data.sh` as a
safety net (idempotent guard `[[ -d data_r02b05 ]] && [[ -n "$(ls -A
...)" ]] || bash tools/download_data.sh`), but pre-fetching keeps the
GPU job focused on compute.

### Step 1 — submit the layered sweep

`run_{daint,beverin}.sh` is the single entry point. It symlinks the
shipped stage 4 SDFGs (`SDFGs/stage4/` → `codegen/stage4/`) and runs
**two layered sweeps**:

1. **Always-on named ablations** via `utils.stages.stage5a` →
   `velocity_stage5a_<CFG>` binaries. Default set:
   `unpermuted_lv{0,1}_sm{0,1}`, `nlev_first_lv{0,1}_sm{0,1}`,
   `index_only_lv{0,1}_sm{0,1}` (8 configs). Override with
   `NAMED_CONFIGS="..."`.
2. **Empirical winners cross-product** via
   [`tools/run_layout_configs.py`](tools/run_layout_configs.py) →
   `velocity_stage6_v123_*` binaries. Reads
   `E6/full_velocity_tendencies/layout_crossproduct_winners.json`
   (regenerated from the loopnest CSVs at `--nlev=$WINNERS_NLEV`,
   default 256, when missing or `REGEN_WINNERS=1`); produces 64 unique
   permute-map signatures from the 3⁶ = 729 raw cells.

Each binary then runs once per timestep listed in `${TIMESTEPS}`
(default `7,9`); per-timestep CSVs land at
`results/<platform>/<config>/ts<N>/`.

```bash
cd Experiments/E7_FullVelocityTendencies
sbatch run_daint.sh        # NVIDIA H200 / GH200
sbatch run_beverin.sh      # AMD MI300A
```

Override knobs (env vars, all optional):

| Knob | Default | Purpose |
|---|---|---|
| `NAMED_CONFIGS` | 8 unpermuted/nlev_first/index_only variants | Replace the always-on ablation set. |
| `WINNERS_JSON` | `…/E6/full_velocity_tendencies/layout_crossproduct_winners.json` | Point at a hand-edited cross-product JSON. |
| `WINNERS_NLEV` | `128` | nlev slice the regenerator reads from the loopnest CSVs. |
| `REGEN_WINNERS` | `0` | Force regeneration of `WINNERS_JSON` even if it exists. |
| `TIMESTEPS` | `7,9` | Comma-separated timestep list for the per-binary runs. |
| `REPS` / `WARMUP` | `100` / `5` | Per-binary timing reps + untimed warm-ups. |
| `--verify` (CLI flag) | off | Keep per-timestep `*.got` / `*.want` blobs (O(GB)) for offline reference comparison. |

To inspect the ablation / cross-product config sets without launching:

```bash
python tools/list_layout_configs.py                                # named configs available to stage5a
python tools/run_layout_configs.py --dry-run \
    --v123-json ../E6_VelocityTendencies/full_velocity_tendencies/layout_crossproduct_winners.json
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
│   ├── list_layout_configs.py               inspector: prints named configs (stage5a)
│   └── run_layout_configs.py                consumes E6's winners cross-product JSON; emits velocity_stage6_v123_* binaries
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
