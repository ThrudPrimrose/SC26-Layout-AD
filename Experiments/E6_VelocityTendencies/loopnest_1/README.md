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

# 4. Build the µ table and plot
python cost_metrics.cpp            # (compiled inside run_*.sh)
python tabularize_metrics.py
python plot.py
```

`run_*.sh` sources `../../common/activate.sh` then
`../../common/setup_{daint,beverin}.sh`, builds `bench_cpu` /
`bench_gpu`, computes µ tables via `cost_metrics`, and writes CSVs
to `results/{daint,beverin}/`.

## Files

- `bench_cpu.cpp` / `bench_gpu.cu` / `bench_gpu_hip.cpp` — benchmark
  drivers under each layout candidate.
- `bench_common.h` — shared indirection + data-loading helpers.
- `icon_data_loader.h` — reads serialized R02B05 tables.
- `cost_metrics.cpp` — computes µ per layout (Table IV).
- `gen_mu_table.py` / `tabularize_metrics.py` — µ post-processing.
- `plot.py` — produces Figure 12.
- `download_data.sh` — fetches serialized ICON fields from ETH PolyBox.

## Protocol

100 repetitions after 5 warm-ups with a Jacobi cache flush. Bootstrap
95% CIs of the median. Two indirection distributions: exact R02B05
connectivity and uniform-random.

## Outputs

- `results/{daint,beverin}/z_v_grad_w_{cpu,gpu}.csv`.
- `metrics{,90,128}.csv` — µ tables for 64 B / 90-level / 128 B cases.
- `mu_{64B,128B}.pdf`, `violins_nlev96.pdf` — Figure 12.
