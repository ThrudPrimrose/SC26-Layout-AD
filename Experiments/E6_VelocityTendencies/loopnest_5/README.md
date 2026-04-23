# E6 / Loopnest 5 — `vn_ie` horizontal boundary kernel

Representative nest from the **horizontal-only boundary** class
(Nest 2 in `access_analysis/chosen_loopnests.md`). Only the top
(`jk=1`) and bottom (`jk=nlevp1`) slabs are written — there is no
inner vertical loop:

```fortran
DO je = i_startidx, i_endidx
  vn_ie   (je, 1,        jb) = vn(je, 1,    jb) + dt_linit_ubc * upper_boundary_cond(je, jb, 1)
  z_vt_ie (je, 1,        jb) = vt(je, 1,    jb)
  z_kin_hor_e(je, 1,     jb) = 0.5_wp * ( vn(je,1,jb)**2 + vt(je,1,jb)**2 )
  vn_ie   (je, nlevp1,   jb) =  wgtfacq_e(je, 1, jb) * vn(je, nlev,   jb) &
                             +  wgtfacq_e(je, 2, jb) * vn(je, nlev-1, jb) &
                             +  wgtfacq_e(je, 3, jb) * vn(je, nlev-2, jb)
END DO
```

Five edge-shaped 2D arrays (`vn_ie`, `z_vt_ie`, `z_kin_hor_e`, `vn`,
`vt`, all sized `N_e × (nlev+1)`) plus two horizontal 1D arrays
(`upper_boundary_cond` with 2 components, `wgtfacq_e` with 3).

Synthetic simplification: `upper_boundary_cond(:,:,1)` is flattened to
the first slot of a 2-wide horizontal array (`ubc`); `wgtfacq_e` is
flattened to a 3-wide horizontal array (`wgt`). `dt_linit_ubc = 0.125`.
Because there is no vertical loop, tile-layout variants are dropped
(TY is irrelevant); the sweep covers only V1–V4 layouts and 1D
blocking B ∈ {8,16,32,64,128}.

## How to run

```bash
bash ../../common/setup.sh        # once per machine
sbatch run_daint.sh
sbatch run_beverin.sh

# Post-process — roofline correlation + paper figure
python tabularize_metrics.py --target cpu_scalar --runtime results/beverin/vn_ie_boundary_cpu.csv
python gen_mu_table.py --csv results/beverin/metrics_cpu_nl90.csv
python plot_paper.py
```

## Files

- `bench_cpu.cpp` / `bench_gpu.cu` — CPU + GPU drivers (`bench_gpu_hip.cpp` is a thin shim); drivers
  sweeping V1–V4 layouts and B ∈ {8,16,32,64,128}. GPU uses a 1D
  thread-block sweep BX ∈ {64,128,256,512,1024}.
- `bench_common.h` — shared layout indexers, NUMA-aware allocator,
  per-1D-array NUMA redistribute, and horizontal-1D fill helpers.
- `cost_metrics.cpp` — analytic μ / Δ / Δ_NUMA / Δ_max sweep with
  10 references per step (top `jk=0` and bottom `jk=nlevp1-1` slabs,
  three back-references into `vn(jk ∈ {nlev-1,nlev-2,nlev-3})`, and a
  single horizontal-1D reference covering both `ubc`/`wgt` — they share
  the same je-indexed NUMA pattern).
- `icon_data_loader.h` is shared with loopnest_1.
- `gen_mu_table.py` — emits LaTeX table + bar plots of µ per layout
  from the `metrics_*_nl90.csv` output of `cost_metrics`.
- `tabularize_metrics.py` — rank-correlates the analytic metrics
  against measured runtime CSVs.
- `plot_paper.py` — 2×2 violin bandwidth figure with STREAM-peak
  annotations (`KERNEL = "vn_ie_boundary"` at the top of the file).

## Outputs

- `results/{daint,beverin}/vn_ie_boundary_{cpu,gpu}.csv` — timing data.
- `results/{daint,beverin}/metrics_{cpu,gpu}_nl90.csv` — analytic μ tables.
