# E0 — NUMA-aware STREAM peak

Baseline bandwidth ceiling for E1–E6. Scale-add RMW
(`A[i] = s * A[i] + B[i]`, 12 B / element fp32) with NUMA-aware
first-touch on the CPU and a grid-stride kernel on the GPU.

## Run

```bash
bash ../common/setup.sh          # one-time per machine
sbatch run_daint.sh              # or run_beverin.sh
```

## Files

- `bench_stream.cpp` — CPU: `mmap(MAP_NORESERVE)` + `madvise(MADV_HUGEPAGE)` + per-thread first-touch.
- `bench_stream_gpu.cu` — unified CUDA/HIP source; beverin passes it to `hipcc -x hip`.
- Shared `gpu_compat.cuh` (in `../common/`) provides the CUDA↔HIP macro shim.

## Outputs

`results/{daint,beverin}/stream_peak_{cpu,gpu}.csv` —
`device, bw_gbs, bw_tbs, N, bytes_per_iter, reps[, threads]`.

Expected (calibration):

| Platform | CPU       | GPU       |
|---|---|---|
| Daint.Alps | ≈ 1.81 TB/s | ≈ 3.78 TB/s |
| Beverin    | ≈ 1.16 TB/s | ≈ 4.29 TB/s |

## Reviewer hint — `# TODO: VERSION`

Inherits all version pins from the common setup (see
[`../README.md`](../README.md#reviewer-hint----todo-version)).
No DaCe, no vendor libraries, no external data.
