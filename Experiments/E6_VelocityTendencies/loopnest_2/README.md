# E6 / Loopnest 2 — `z_w_concorr_me` direct stencil (partial vertical)

Representative nest from the **direct stencil, partial vertical** class
(Nest 5 in `access_analysis/chosen_loopnests.md`):

```fortran
DO jk = nflatlev_jg, nlev
  DO je = i_startidx, i_endidx
    z_w_concorr_me(je,jk,jb) =
        p_prog%vn(je,jk,jb)        * p_metrics%ddxn_z_full(je,jk,jb)
      + p_diag%vt(je,jk,jb)        * p_metrics%ddxt_z_full(je,jk,jb)
  END DO
END DO
```

Five same-shape edge arrays (`vn`, `vt`, `ddxn_z_full`, `ddxt_z_full`,
`z_w_concorr_me`) and no indirection — purely a layout/blocking probe
on the *partial vertical* range `[nflatlev_jg, nlev)`. Used here as a
counterpoint to loopnest_1's indirect stencil: identical sweep design,
different arithmetic intensity / contention profile.

`nflatlev_jg` is set to `nlev/3` as a representative R02B05 split.

## How to run

```bash
bash ../../common/setup.sh        # once per machine
sbatch run_daint.sh
sbatch run_beverin.sh
```

`run_*.sh` sources `../../common/activate.sh` then
`../../common/setup_{daint,beverin}.sh`, builds `bench_cpu_a` /
`bench_gpu_a`, and runs the cost-metric sweep, writing CSVs to
`results/{daint,beverin}/`.

## Files

- `bench_cpu.cpp` / `bench_gpu.cu` — CPU + GPU drivers (`bench_gpu_hip.cpp` is a thin shim); drivers
  sweeping V1–V4 layouts, blocked B ∈ {8,16,32,64,128}, and tiled
  `(TX,TY) ∈ {8,16,32,64}²`.
- `bench_common.h` — shared layout indexers, NUMA-aware allocator,
  redistribute helpers.
- `cost_metrics.cpp` — analytic μ / Δ / Δ_NUMA / Δ_max sweep with
  5 references per step.
- `icon_data_loader.h` is shared with loopnest_1 via
  `#include "../loopnest_1/icon_data_loader.h"`.

## Protocol

100 timed repetitions after 5 warm-ups, Jacobi cache flush between
runs. Three indirection distributions (uniform, normal_var1, exact —
the last requires the serialized R02B05 fields to be present at
`$ICON_DATA_PATH`).

## Outputs

- `results/{daint,beverin}/z_w_concorr_me_{cpu,gpu}.csv` — timing data.
- `results/{daint,beverin}/metrics_{cpu,gpu}_nl90.csv` — analytic
  μ tables for 64 B / 128 B cache lines at nlev = 90.
