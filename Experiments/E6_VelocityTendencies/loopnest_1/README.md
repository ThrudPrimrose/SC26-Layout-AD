# E6 / Loopnest 1 — `z_v_grad_w` indirect stencil (Figure 12, Table IV)

Representative nest from the "indirect stencil" pattern class.
Benchmarks row-major vs. vertical-first layouts under exact R02B05
connectivity and a uniform-random indirection baseline.

## How to run

```bash
# 1. One-time (from repo root)
bash ../../common/setup.sh

# 2. Fetch the R02B05 connectivity + serialized ICON data
bash download_data.sh

# 3. Submit on your cluster
sbatch run_daint.sh
sbatch run_beverin.sh

# 4. Post-process — µ table + roofline correlation + paper figure
python tabularize_metrics.py --target cpu_scalar --runtime results/beverin/z_v_grad_w_cpu.csv
python gen_mu_table.py --csv results/beverin/metrics_cpu_nl90.csv
python plot_paper.py
```

`run_*.sh` sources `../../common/activate.sh` then
`../../common/setup_{daint,beverin}.sh`, builds `bench_cpu` /
`bench_gpu`, computes µ tables via `cost_metrics`, and writes CSVs
to `results/{daint,beverin}/`.

## Files

- `bench_cpu.cpp` / `bench_gpu.cu` — CPU + GPU benchmark sources (`bench_gpu_hip.cpp` is a thin shim); benchmark
  drivers under each layout candidate.
- `bench_common.h` — shared indirection + data-loading helpers.
- `icon_data_loader.h` — reads serialized R02B05 tables.
- `cost_metrics.cpp` — analytic µ / Δ / Δ_NUMA / Δ_max sweep (Table IV).
- `gen_mu_table.py` — emits LaTeX table + bar plots of µ per layout
  from the `metrics_*_nl90.csv` output of `cost_metrics`.
- `tabularize_metrics.py` — rank-correlates the analytic metrics
  (Spearman/Kendall/Pearson) against the measured runtime CSVs.
- `plot_paper.py` — produces the 2×2 violin bandwidth figure (one
  panel per platform × device); annotates medians as % of STREAM
  peak. `KERNEL = "z_v_grad_w"` at the top is the only line that
  changes across loopnest_1..6.
- `download_data.sh` — fetches serialized ICON fields from ETH PolyBox.

## Protocol

100 repetitions after 5 warm-ups with a Jacobi cache flush. Bootstrap
95% CIs of the median. Two indirection distributions: exact R02B05
connectivity and uniform-random.

## Outputs

- `results/{daint,beverin}/z_v_grad_w_{cpu,gpu}.csv`.
- `metrics{,90,128}.csv` — µ tables for 64 B / 90-level / 128 B cases.
- `mu_{64B,128B}.pdf`, `violins_nlev128.pdf` — Figure 12 (default nlev=128; the plotter also emits `violins_nlev{90,96,256}.pdf` if the data is present).
