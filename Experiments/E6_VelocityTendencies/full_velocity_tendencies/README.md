# E6 / Full Velocity Tendencies (Task T6.5)

End-to-end evaluation of the ICON velocity-tendencies module as a DaCe
SDFG, including the stage-by-stage lowering pipeline and the full
layout-permutation sweep at stage 4 (CPU) and stage 8 (GPU).

Artifacts in this directory are lifted from the `sc26` branch of
[`spcl/icon-artifacts`](https://github.com/spcl/icon-artifacts) (working
tree at `~/Work/icon-artifacts/velocity`). DaCe is expected to be on
branch `f2dace/staging`, which carries the layout-transformation passes
cherry-picked from `yakup/dev`. The cherry-pick recipe lives in
[sc26_layout/prepare.sh](sc26_layout/prepare.sh).

## Layout

```
full_velocity_tendencies/
├── README.md                    (this file)
├── run_daint.sh                 # clean submission script (stage4 + stage8)
├── run_beverin.sh               # idem for Beverin (MI300A)
│
├── SDFGs/                       # archived SDFG baselines
│   ├── baselines/               #   top-level SDFGs (pre-lowering, concretized)
│   ├── stages/                  #   per-stage checkpoints (stage1 .. stage9)
│   └── stage_programs/          #   final stage5/stage8 program.sdfg
│       │                            gzipped per-config
│       ├── stage5_unpermuted_wnuma/
│       ├── stage5_unpermuted_wonuma/
│       ├── stage8_unpermuted/
│       ├── stage8_permuted_nlev_first_ms/
│       ├── stage8_permuted_nlev_first_mu/
│       ├── stage8_permuted_index_only_ms/
│       └── stage8_permuted_index_only_mu/
│
├── sc26_layout/                 # layout-transformation research code
│   ├── permute_stage4.py        #   95-config CPU permutation matrix
│   ├── permute_stage8.py        #   95-config GPU permutation matrix
│   ├── plot_stage{6,8}.py       #   per-stage visualization
│   ├── extract_{gpu_,}kernel.py #   kernel extraction helpers
│   ├── velocity_layout.md       #   layout dimension groups documentation
│   ├── velocity_annotated.f90   #   annotated Fortran reference
│   ├── Microbenchmarks/         #   hand-written wcon / transpose benches
│   └── r02b05_ln/, r02b06_ln/   #   reference profiling output
│
└── scripts/                     # build + compile + run + plot utilities
    ├── run_stage4_permutations.py   # CPU driver (95 configs × ms/mu × wnuma/wonuma)
    ├── run_stage8_permutations.py   # GPU driver
    ├── run_permutations.py          # shared helpers
    ├── run_cpu_permutations.py      # CPU-only sweep driver
    ├── utils/                       # DaCe transformation library (required by runners)
    │   └── stages/compile_gpu_stageN.py  # per-stage lowering pipelines
    ├── sc26_layout -> ../sc26_layout  (symlink, so runners can `import sc26_layout.*`)
    ├── build*.sh, build_baseline.py
    ├── compile_velocity_gpu.py, collect_headers.py
    ├── bench_stage{4,8}.sh, bench_loopnest*.sh
    ├── download_nproma{32,20480}_data.sh
    ├── nsys_stage8.sh, profile_stage{6_single_map,8}.sh
    ├── plotting/                    # all plot_*.py + plot_all_permutations.sh
    └── legacy_submission/           # original run_full_permutations_*.sh from
                                     # icon-artifacts for reference
```

## Stage pipeline

1. **stage1 .. stage3** — lowering from Fortran AST → DaCe SDFG.
2. **stage4 (CPU)** — first executable stage; CPU codegen.
3. **stage5** — NUMA-aware CPU variant (`_wnuma` vs `_wonuma`).
4. **stage6 .. stage7** — intermediate GPU lowering.
5. **stage8 (GPU)** — terminal GPU codegen; compiles to `_ms`/`_mu`
   executables (map-shuffled vs. unshuffled).
6. **stage9** — post-codegen cleanup.

The stage-4 and stage-8 drivers sweep the 95-config × {ms,mu} × {wnuma,
wonuma} permutation matrix defined in
[sc26_layout/permute_stage4.py](sc26_layout/permute_stage4.py) and
[sc26_layout/permute_stage8.py](sc26_layout/permute_stage8.py). Five
dimension groups are varied independently:

| key | arrays | choices |
|---|---|---|
| `cv` | `z_w_con_c`, `z_w_concorr_mc`, `z_w_v` | identity vs. level-first |
| `ch` | `z_ekinh`, `z_kin_hor_e` | identity vs. level-first |
| `f`  | `vn`, `w`, `vt`, `vn_ie`, `ddt_*` | identity vs. level-first |
| `s`  | `wgtfac_e`, `ddxn`, `coeff_*` | identity vs. level-first |
| `n`  | connectivity indices | 6 permutations of `[0,1,2]` |

Named sub-sweeps: `nlev_first`, `index_only`, `unpermuted`, plus the
full 95-config sweep (`CONFIGS=full_sweep`).

## How to run

```bash
# 1. One-time (from repo root) with the layout-transformation DaCe branch
DACE_BRANCH=f2dace/staging bash ../../common/setup.sh

# 2. Fetch R02B05 data (if not already present)
bash scripts/download_nproma20480_data.sh

# 3. Submit (stage4 + stage8 by default; overrideable)
sbatch run_daint.sh                                      # NVIDIA H200 / GH200
sbatch run_beverin.sh                                    # AMD MI300A
#   REPS=200 CONFIGS="full_sweep" sbatch run_daint.sh
#   STAGE_SET="8" CONFIGS="nlev_first" sbatch run_beverin.sh

# 4. Plot
python scripts/plotting/plot_stage5_8_combined.py
```

## Outputs

- `results/{daint,beverin}/stage4/` — stage-4 CPU permutation timings.
- `results/{daint,beverin}/stage8/` — stage-8 GPU permutation timings.
- Inner layout: runners write per-config `*.txt` files and a summary CSV.

## Inputs

- `../conflict_resolution/selected_layouts.json` — canonical layout
  choices fed forward from the conflict-resolution analysis.
- R02B05 ICON dataset (fetched by `scripts/download_nproma20480_data.sh`).
