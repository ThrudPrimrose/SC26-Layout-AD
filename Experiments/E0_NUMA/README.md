# E0 — NUMA-aware STREAM sweep

2D bandwidth baseline for E1–E6. Scale-add RMW
(`A[y,x] = s * A[y,x] + B[y,x]`, 12 B / fp32) sweeping:

- **CPU**: 4 default 1D OMP schedules + a 2×2 NUMA-quadrant first-touch
  layout with an inner `TILE_Y × TILE_X` cache-blocking sweep.
- **GPU**: templated `(BX, BY, TX, TY)` kernels — block shape × per-thread
  tile — with strided indexing so coalescing holds even for `TX > 1`.

Models match `E1_MatrixAdd/bench_*.cu` conventions: shared NUMA helpers
and `flush_jacobi()` from `common/`, `NREP=100 / NWARMUP=5`, checksum
validation, one CSV row per repetition for violin plots.

## Run

```bash
bash ../common/setup.sh          # one-time per machine
sbatch run_daint.sh              # or run_beverin.sh
python plot_paper.py             # pretty-prints best-per-category
```

## Files

- `bench_stream.cpp` — CPU sweep (`1d_rows_static`, `1d_collapse_static`,
  `1d_flat_static`, `1d_rows_dynamic`, `2x2_numa_tiled[TILE_Y,TILE_X]`).
- `bench_stream_gpu.cu` — GPU `(BX, BY, TX, TY)` sweep. On beverin we
  pass the `.cu` directly to `hipcc -x hip` (no separate `_hip.cpp`).
- Shared headers live in [`../common/`](../common/): `numa_util.h`,
  `jacobi_flush.h`, `gpu_compat.cuh`.
- `run_daint.sh`, `run_beverin.sh`, `plot_paper.py`.

## Protocol

100 reps after 5 warm-ups. CPU flushes cache with the canonical
`flush_jacobi()` between every rep; GPU relies on the kernel's own
pressure on L2 (no explicit flush). Correctness: analytic checksum
(after one RMW from `A=1.0, B=2.0`, every element is `3.0001`).

## Outputs

- `results/{daint,beverin}/stream_peak_cpu.csv` — per-rep:
  `variant,TILE_Y,TILE_X,N,rep,threads,time_s,bw_gbs,checksum,status`.
- `results/{daint,beverin}/stream_peak_gpu.csv` — per-rep:
  `kernel,BX,BY,TX,TY,N,rep,time_ms,bw_gbs,checksum,status`.

Expected best-case numbers (calibration; see
[`../common/stream_peak.json`](../common/stream_peak.json) for the
authoritative values used by every other plotter):

| Platform | CPU         | GPU         |
|---|---|---|
| Daint.Alps | ≈ 1.614 TB/s | ≈ 3.783 TB/s |
| Beverin    | ≈ 0.968 TB/s | ≈ 3.551 TB/s |

The Beverin GPU value is the post-calibration peak; the submitted
paper used ≈ 4.29 TB/s, inflated because the cache-flush kernel was
being dead-code-eliminated by the GPU compiler. See the top-level
README's *Calibration update* section.

## Reviewer hint — `# TODO: VERSION`

Inherits common pins (see
[`../README.md`](../README.md#reviewer-hint----todo-version)).
No DaCe, no vendor libraries, no external data. Two sweep dimensions
are hard-coded and may want tuning:

- CPU: `TILE_Ys[]` and `TILE_Xs[]` in `bench_stream.cpp`.
- GPU: `register_configs()` in `bench_stream_gpu.cu` — each
  `reg_config<BX,BY,TX,TY>()` call is a compile-time instantiation.
