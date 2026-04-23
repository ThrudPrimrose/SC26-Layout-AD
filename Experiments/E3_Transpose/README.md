# E3 — Matrix Transpose (Figure 9, Table III)

`N × N` fp32 transpose, row-major vs. blocked layouts. DaCe-produced
variants vs. vendor libraries: HPTT (CPU), cuTENSOR (Daint GPU),
hipTensor (Beverin GPU).

## Run

```bash
bash ../common/setup.sh          # one-time per machine
sbatch run_daint.sh              # or run_beverin.sh
python plot_paper.py
python delta_table.py            # Table III (block-distance reduction)
```

## Files

- `transpose_cpu.cpp`, `transpose_gpu.cu`, `transpose_gpu_hip.cpp` —
  DaCe variants.
- `transpose_hptt.cpp` — HPTT CPU baseline.
- `transpose_cutensor.cu` (Daint), `transpose_hiptensor.cpp` (Beverin)
  — vendor GPU baselines.
- `transpose_openblas.cpp` — OpenBLAS CPU baseline.
- `cost_metrics.cpp` — µ / Δ per layout candidate.
- `plot_paper.py`, `delta_table.py` — Figure 9, Table III.

## Protocol

100 reps after 5 warm-ups, Jacobi cache flush, bootstrap 95 % CI of
the median.

## Outputs

- `results/{daint,beverin}/transpose_{cpu,gpu}_{raw,results}.csv`.
- `transpose_metrics_{cpu,gpu}.csv` — µ, Δ per candidate.
- `transpose_paper.pdf` (Figure 9), `metrics_table.tex` (Table III).

## Data loading

None — `N × N` fp32 generated in-process.

## Reviewer hint — `# TODO: VERSION`

Version-sensitive beyond the common pins (see
[`../README.md`](../README.md#reviewer-hint----todo-version)):

- **HPTT**, **cuTENSOR** (Daint), **hipTensor** (Beverin) — loaded
  from spack by `setup_{daint,beverin}.sh`.
- **OpenBLAS** — 0.3.29 (Daint) / 0.3.30 (Beverin).

`cost_metrics.cpp` is vendor-independent; re-run the full sweep only
if a vendor library is upgraded.
