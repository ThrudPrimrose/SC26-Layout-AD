# E6 / Loopnest 3 — `z_v_grad_w` direct stencil (full vertical)

Representative nest from the **direct stencil, full vertical** class
(Nest 7 in `access_analysis/chosen_loopnests.md`):

```fortran
DO jk = 1, nlev
  DO je = i_startidx, i_endidx
    z_v_grad_w(je,jk,jb) = z_v_grad_w(je,jk,jb)*deepatmo_gradh_ifc(jk)
                         + vn_ie(je,jk,jb)*(vn_ie(je,jk,jb)*deepatmo_invr_ifc(jk)
                                            - p_patch%edges%ft_e(je,jb))
                         + z_vt_ie(je,jk,jb)*(z_vt_ie(je,jk,jb)*deepatmo_invr_ifc(jk)
                                            + p_patch%edges%fn_e(je,jb))
  END DO
END DO
```

Seven arrays:

- 3 same-shape edge 2D arrays — `z_v_grad_w` (RMW), `vn_ie`, `z_vt_ie`
- 2 vertical-only 1D arrays — `deepatmo_gradh_ifc(jk)`, `deepatmo_invr_ifc(jk)`
- 2 horizontal-only 1D arrays — `ft_e(je)`, `fn_e(je)`

Counterpoint to loopnest_2 (same direct-stencil family but full vertical
range and added 1D-vertical / 1D-horizontal accesses) and to loopnest_1
(same full-vertical range but no neighbour indirection).

## How to run

```bash
bash ../../common/setup.sh        # once per machine
sbatch run_daint.sh
sbatch run_beverin.sh
```

## Files

- `bench_cpu.cpp` / `bench_gpu.cu` — CPU + GPU drivers (`bench_gpu_hip.cpp` is a thin shim); drivers
  sweeping V1–V4 layouts, B ∈ {8,16,32,64,128}, (TX,TY) ∈ {8,16,32,64}².
- `bench_common.h` — shared layout indexers, NUMA-aware allocator, and
  per-1D-array NUMA redistribute.
- `cost_metrics.cpp` — analytic μ / Δ / Δ_NUMA / Δ_max sweep with
  7 references per step.
- `icon_data_loader.h` is shared with loopnest_1.

## Outputs

- `results/{daint,beverin}/z_v_grad_w_full_{cpu,gpu}.csv` — timing data.
- `results/{daint,beverin}/metrics_{cpu,gpu}_nl90.csv` — analytic μ tables.
