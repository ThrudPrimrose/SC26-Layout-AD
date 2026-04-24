# E6 — ICON Velocity Tendencies (Figures 12–13, Table IV)

End-to-end optimization of the ICON velocity-tendencies module (25
loop nests, 50 arrays): canonicalize → score with µ/Δ → per-class
benchmark → resolve conflicts → full-module sweep.

Uses **two DaCe branches**: `yakup/dev` everywhere except
`full_velocity_tendencies/` which needs `f2dace/staging`.

## Pipeline

| Task | Folder | Script |
|---|---|---|
| T6.1 Data | `loopnest_1/` | `bash download_data.sh` |
| T6.2 LOKI access analysis | [`access_analysis/`](access_analysis/) | `python analyze_access.py velocity_advection_preprocessed.f90` → `*.analysis.md` |
| T6.3 Representative loopnest selection | [`access_analysis/`](access_analysis/) | `python select_loopnests.py` → `layout_candidates.json` + `chosen_loopnests.md` |
| T6.4 Canonical 5-group taxonomy | [`access_analysis/`](access_analysis/) | `python generate_canonical_groups.py` → `canonical_array_groups.json` |
| T6.5 Per-loopnest sweep | [`loopnest_{1..6}/`](loopnest_1/) | `sbatch run_{daint,beverin}.sh` |
| T6.6 Blocked-vs-unblocked verdict | (root) | `python analyze_blocked_vs_unblocked.py --backend {gpu,cpu,all} --nlev 256 --json blocked_verdict.json` |
| T6.7 Build layout cross-product | (root) | `python build_layout_crossproduct.py --backend {gpu,cpu,all} --nlev 256 --blocked-verdict blocked_verdict.json` → `full_velocity_tendencies/layout_crossproduct.{json,csv}` |
| T6.8 Full-module sweep | [`full_velocity_tendencies/`](full_velocity_tendencies/) | `sbatch run_{daint,beverin}.sh` (reads `layout_crossproduct.csv`) |
| T6.9 Plots | subfolders | `loopnest_1/plot_paper.py` (Fig. 12 / Tab. IV); `full_velocity_tendencies/scripts/plotting/plot_stage5_8_combined.py` (Fig. 13) |

### LOKI → loopnests → groups → axis-projected sweep

1. **LOKI** (`analyze_access.py`) parses `velocity_tendencies` and extracts
   25 leaf loop nests with role-tagged access patterns (`h:U v:S b:S`,
   `h:S b:S`, …). Output: `*.analysis.md`.
2. **Representative selection** (`select_loopnests.py`) groups nests by
   pattern class and picks one per class (widest, `full_vert` preferred).
   Writes the 6 chosen nests into `layout_candidates.json` +
   `chosen_loopnests.md`.
3. **Canonical 5-group taxonomy** (`generate_canonical_groups.py`) derives
   cv / ch / f / s / n from the per-nest array access signatures,
   tagging each group with its storage axis (`IC` or `IN`). Connectivity
   aliases are force-added to `n` because LOKI's `INDIRECT_ARRAYS` set
   strips them from every nest's array list.
4. **Per-loopnest benchmarks** (`loopnest_{1..6}/`) sweep layout
   variants V1..V7 + blocked factors B8..B128 at nlev=256, over
   `cell_dist ∈ {uniform, normal_var1, exact}`.
5. **Blocked verdict** (`analyze_blocked_vs_unblocked.py`) compares the
   best blocked median against the best unblocked median per loopnest ×
   target. A loopnest keeps blocked iff max-speedup ≥ 1.25×; otherwise
   blocked is dropped from its winner pool.
6. **Cross-product builder** (`build_layout_crossproduct.py`) ranks V-ids
   per loopnest by the geomean of normalized medians, **projects each
   V-id onto its storage-axis components** (`V1` → IC:h_first/IN:SoA;
   `V6` → IC:v_first/IN:AoS; …), and votes for axis values on the
   axis each group consumes. Each group's candidates are axis values
   (`h_first`/`v_first` or `SoA`/`AoS`), not V-ids. Groups with no
   touching loopnest (e.g. `n`, because LOKI can't see it) fall back to
   the full axis (both choices).
7. **Full sweep** (`full_velocity_tendencies/`) consumes
   `layout_crossproduct.csv` (one row = one DaCe permutation config) and
   benchmarks each one end-to-end.

#### Why axis projection?

A micro-bench V-id encodes two independent storage choices:
  * **IC axis** (field innermost): `V1,V2 → h_first`;
    `V3..V7 → v_first`.
  * **IN axis** (2-col connectivity): `V{odd} → SoA`;
    `V{even} → AoS`.

V5/V6/V7 differ from V3/V4 only in the GPU kernel schedule, not in
storage — so at the full-sweep level they alias. Projecting to axes
before voting eliminates the alias, keeps the candidate set minimal
(2 per axis), and ensures each group sees only the axis its arrays
actually have.

## Run a subtask

```bash
bash ../../common/setup.sh       # one-time (repo root)
cd loopnest_1                    # or any other subtask
sbatch run_daint.sh              # or run_beverin.sh
```

Full-module stage needs the other DaCe branch:

```bash
DACE_BRANCH=f2dace/staging source ../../common/activate.sh
```

## Status

- ✅ `loopnest_{1..6}` — CPU + GPU benchmarks (single-source CUDA/HIP via `common/gpu_compat.cuh`) + `cost_metrics.cpp`.
- ✅ `access_analysis` — LOKI-driven loop-nest + array-group extraction.
- ✅ Blocked-vs-unblocked analyzer + layout cross-product builder at the
  E6 root (`analyze_blocked_vs_unblocked.py`, `build_layout_crossproduct.py`).
- 🚧 `full_velocity_tendencies` — stage4 / stage8 permutation sweep,
  mirrored from R2 (`spcl/icon-artifacts`, `f2dace/staging`). Driven by
  `layout_crossproduct.csv`.

## Outputs

- `loopnest_{1..6}/results/{daint,beverin}/*.csv` (Figure 12, Table IV).
- `full_velocity_tendencies/results/{daint,beverin}/stage{4,8}/*.csv`
  (Figure 13).

## Data loading

- `loopnest_1/download_data.sh` — ICON R02B05 serialized input (≈ 3 GB),
  shared across **all** `loopnest_{1..6}`.
- `full_velocity_tendencies/scripts/download_nproma20480_data.sh` —
  full-module `nproma=20480` dataset (≈ 8 GB).

Outbound HTTPS required; ≈ 11 GB scratch quota.

## Reviewer hint — `# TODO: VERSION`

Common pins — see
[`../README.md`](../README.md#reviewer-hint----todo-version). E6-specific:

- **DaCe branch** — `yakup/dev` vs. `f2dace/staging` (see above).
- **Dataset URLs** in the two `download_data.sh`'s (override with
  `DATA_URL=<mirror>`).
- **ICON commit** used to serialize inputs — pin the same SHA if you
  rebuild the dataset, or outputs won't be bit-identical.
