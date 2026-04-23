# E6 / Loopnest 6 — `levelmask` level reduction

Representative nest from the **vertical-only (level reduction)** class
(Nest 25 in `access_analysis/chosen_loopnests.md`):

```fortran
DO jk = MAX(3, nrdmax_jg - 2), nlev - 3
  levelmask(jk) = ANY(levmask(i_startblk:i_endblk, jk))
END DO
```

Two arrays only: `levmask` (edge-shaped 2D, read) and `levelmask`
(vertical 1D, written).

Synthetic simplification: `levmask` is an edge-shaped `N_e × nlev`
mask stored as `double` (0.0 = false, 1.0 = true); the "ANY" reduction
is emulated by initializing `levelmask` to 0.0 and having every thread
that sees a "true" input write 1.0 (race-free since all concurrent
writers write the same value — no atomics). Partial-vertical range
`[max(3, nlev/8), nlev-3)` stands in for `MAX(3, nrdmax-2) .. nlev-3`.
Mask density is chosen sparse (`MASK_P_TRUE = 0.15`) so the ANY
reduction is not trivially saturated.

## How to run

```bash
bash ../../common/setup.sh        # once per machine
sbatch run_daint.sh
sbatch run_beverin.sh

# Post-process — roofline correlation + paper figure
python tabularize_metrics.py --target cpu_scalar --runtime results/beverin/levelmask_cpu.csv
python gen_mu_table.py --csv results/beverin/metrics_cpu_nl90.csv
python plot_paper.py
```

## Files

- `bench_cpu.cpp` / `bench_gpu.cu` — CPU + GPU drivers (`bench_gpu_hip.cpp` is a thin shim); drivers
  sweeping V1–V4 layouts, B ∈ {8,16,32,64,128}, (TX,TY) ∈ {8,16,32,64}².
  Each CPU variant runs a `jk_outer` and a `collapse2` schedule; GPU
  sweeps BX ∈ {64,128,256,512}.
- `bench_common.h` — shared layout indexers (V/B/T), NUMA-aware
  allocator, `levmask` redistribute helpers, and the mask-fill helper.
- `cost_metrics.cpp` — analytic μ / Δ / Δ_NUMA / Δ_max sweep with
  NR=1 reference per je (one load of `levmask(je, jk)` per iteration);
  the small 1D `levelmask` output is excluded from the reference set
  because it is NUMA-replicable. Slice partitioning follows the
  kernel's `jk_outer` parallelism.
- `icon_data_loader.h` is shared with loopnest_1.
- `gen_mu_table.py` — emits LaTeX table + bar plots of µ per layout
  from the `metrics_*_nl90.csv` output of `cost_metrics`.
- `tabularize_metrics.py` — rank-correlates the analytic metrics
  against measured runtime CSVs.
- `plot_paper.py` — 2×2 violin bandwidth figure with STREAM-peak
  annotations (`KERNEL = "levelmask"` at the top of the file).

## Outputs

- `results/{daint,beverin}/levelmask_{cpu,gpu}.csv` — timing data.
- `results/{daint,beverin}/metrics_{cpu,gpu}_nl90.csv` — analytic μ tables.
