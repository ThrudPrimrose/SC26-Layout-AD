# E6 / Loopnest 4 — `ddt_vn_vert` indirect stencil + CFL clip

Representative nest from the **indirect stencil, partial vertical,
CFL-gated** class (Nest 24 in `access_analysis/chosen_loopnests.md`):

```fortran
DO jk = MAX(3,nrdmax-2), nlev-4
  IF (levmask(jk,jb) .OR. levmask(jk+1,jb)) THEN
    DO je = i_startidx, i_endidx
      w_con_e = c_lin_e(je,1,jb)*z_w_con_c(icidx(je,jb,1), jk, icblk(je,jb,1)) &
              + c_lin_e(je,2,jb)*z_w_con_c(icidx(je,jb,2), jk, icblk(je,jb,2))
      IF (ABS(w_con_e) > cfl_w_limit*ddqz_z_full_e(je,jk,jb)) THEN
        difcoef = sfex*MIN(0.85_wp-cfl_w_limit*dtime, &
                           ABS(w_con_e)*dtime/ddqz_z_full_e(je,jk,jb) - cfl_w_limit*dtime)
        ddt_vn_apc(je,jk,jb) = ddt_vn_apc(je,jk,jb) + difcoef*p_patch%edges%area(je,jb) * ( &
           geofac_grdiv(je,1,jb)*vn(je,jk,jb) &
         + geofac_grdiv(je,2,jb)*vn(iqidx(je,jb,1),jk,iqblk(je,jb,1)) &
         + geofac_grdiv(je,3,jb)*vn(iqidx(je,jb,2),jk,iqblk(je,jb,2)) &
         + geofac_grdiv(je,4,jb)*vn(iqidx(je,jb,3),jk,iqblk(je,jb,3)) &
         + geofac_grdiv(je,5,jb)*vn(iqidx(je,jb,4),jk,iqblk(je,jb,4)) &
         + p_patch%edges%tangent_orientation(je,jb) * p_patch%edges%inv_primal_edge_length(je,jb) &
         * (zeta(ividx(je,jb,2),jk,ivblk(je,jb,2)) - zeta(ividx(je,jb,1),jk,ivblk(je,jb,1))) )
      END IF
    END DO
  END IF
END DO
```

10 arrays (5 edge-shaped 2D + 5 horizontal 1D) plus three indirection
tables (`icidx`, `iqidx`, `ividx`) and one vertical mask (`levmask`).

Synthetic simplification: cell/vertex/edge distinction is flattened to
edge-shaped 2D arrays; neighbour indices are random flat edge-ids in
`[0, N_e)`. Partial-vertical range `[nlev/8, 7*nlev/8)` stands in for
`MAX(3, nrdmax-2) .. nlev-4`. The `levmask` random fill uses `p_true = 0.6`.

## How to run

```bash
bash ../../common/setup.sh        # once per machine
sbatch run_daint.sh
sbatch run_beverin.sh

# Post-process — roofline correlation + paper figure
python tabularize_metrics.py --target cpu_scalar --runtime results/beverin/ddt_vn_vert_cpu.csv
python gen_mu_table.py --csv results/beverin/metrics_cpu_nl90.csv
python plot_paper.py
```

## Files

- `bench_cpu.cpp` / `bench_gpu.cu` — CPU + GPU drivers (`bench_gpu_hip.cpp` is a thin shim); drivers
  sweeping V1–V4 layouts, B ∈ {8,16,32,64,128}, (TX,TY) ∈ {8,16,32,64}².
- `bench_common.h` — shared layout indexers, NUMA-aware allocator,
  per-1D-array NUMA redistribute, and indirection-table / level-mask
  fill helpers.
- `cost_metrics.cpp` — analytic μ / Δ / Δ_NUMA / Δ_max sweep with
  10 references per step (indirect neighbour blocks intentionally
  excluded — they are dominated by the random indirection pattern and
  would mask the layout-sensitive signal).
- `icon_data_loader.h` is shared with loopnest_1.
- `gen_mu_table.py` — emits LaTeX table + bar plots of µ per layout
  from the `metrics_*_nl90.csv` output of `cost_metrics`.
- `tabularize_metrics.py` — rank-correlates the analytic metrics
  against measured runtime CSVs.
- `plot_paper.py` — 2×2 violin bandwidth figure with STREAM-peak
  annotations (`KERNEL = "ddt_vn_vert"` at the top of the file).

## Outputs

- `results/{daint,beverin}/ddt_vn_vert_{cpu,gpu}.csv` — timing data.
- `results/{daint,beverin}/metrics_{cpu,gpu}_nl90.csv` — analytic μ tables.
