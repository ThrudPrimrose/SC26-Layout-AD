# Supplemental / NumaStream

Bandwidth sweep for `C = alpha * (A + B)` on very large fp64 matrices.
Two dimensions:

- **CPU**: compares plain parallel first-touch vs. explicit 4-NUMA-tile
  binding vs. `MPOL_INTERLEAVE` across matrix sizes.
- **GPU**: sweeps the thread-block size × grid-multiplier Cartesian
  product across matrix sizes.

This is a supplemental measurement (does not map directly to a paper
figure) that quantifies the pure-bandwidth ceiling of the shared
infrastructure under `common/`.

## Kernel

```c
C[i] = alpha * (A[i] + B[i])
```

Traffic: 2 loads + 1 store per fp64 element = **24 B / element**. All
three buffers are allocated via `numa_alloc_unfaulted<double>` from
[`common/numa_util.h`](../../common/numa_util.h); page placement is
driven by the variant under test.

## Sweep configuration

| Axis | Values |
|---|---|
| N (matrix edge)       | `{8192, 16384, 24576}` → 512 MiB / 2 GiB / 4.5 GiB per buffer |
| CPU variant           | `baseline_ft`, `numa{D}_stripe` (D = min(detected nodes, 4)), `interleave` |
| GPU block size        | `{64, 128, 256, 512, 1024}` |
| GPU grid multiplier m | `{2, 4, 8, 16, 32}` (grid = SMs × m) |
| Warmup iterations     | 5   (override with `WARMUP=…`) |
| Timed iterations      | 100 (override with `NRUNS=…`) |
| Scalar α              | `1.0001` (override with `ALPHA=…`) |

Between every timed rep both benches invoke the canonical 8192² 3-step
Jacobi flush — the CPU side uses
[`common/jacobi_flush.h`](../../common/jacobi_flush.h) and the GPU side
uses the twin
[`common/jacobi_flush_gpu.cuh`](../../common/jacobi_flush_gpu.cuh) (same
size, same sweep count, same buffer-swap pattern; allocated once per
process and reused). This is the same convention as every E1–E6 bench.

## How to run

```bash
# one-time per machine (from repo root)
bash ../../common/setup.sh

# submit
sbatch run_daint.sh         # Grace + Hopper
sbatch run_beverin.sh       # Zen 4 + MI300A

# env overrides (optional)
NLIST="16384,32768" REPS=100 sbatch run_daint.sh
```

Each run script sources `../../common/activate.sh` and
`../../common/setup_{daint,beverin}.sh`, then compiles `bench_cpu.cpp`
and `bench_gpu.cu` (or `bench_gpu_hip.cpp` on Beverin) against the
shared `CPU_CXX`, `CPU_CXXFLAGS`, `GPU_CXX`, `GPU_CXXFLAGS` from the
platform env.

## Outputs

```
results/
├── daint/
│   ├── numa_cpu.csv         # one row per (variant, N)
│   ├── numa_gpu.csv         # one row per (block, grid_mult, N)
│   └── SUP_numa_daint_<jobid>.{out,err}
└── beverin/
    └── (same)
```

### CSV schemas

`numa_cpu.csv` (tall — one row per rep, `nruns` rows per variant-N pair):
```
device,variant,N,elems,bytes_per_iter,threads,numa_nodes,warmup,nruns,run_id,bw_gbs,bw_tbs,alpha
```

`numa_gpu.csv` (tall — one row per rep, `nruns` rows per combo):
```
device,N,elems,bytes_per_iter,block,grid_mult,grid,warmup,nruns,run_id,bw_gbs,bw_tbs,alpha,sm_count
```

Plot scripts should load these via
[`common/plot_util.py`](../../common/plot_util.py):
```python
from plot_util import load_csv, PLATFORMS, experiment_dir
EXP_DIR = experiment_dir(__file__)
df = load_csv(EXP_DIR, "numa_cpu.csv")   # all platforms concatenated
```

## Plot

```bash
python plot_paper.py
```

Produces:
- `numa_cpu.pdf` — bar chart (bandwidth vs variant, grouped by N),
  one panel per platform.
- `numa_gpu.pdf` — heatmap (bandwidth vs block × grid_mult), one panel
  per (platform, N). The peak point in each heatmap is boxed.

## Expected behaviour

- `baseline_ft` and `numa4_stripe` should be close on a well-pinned run
  (`OMP_PROC_BIND=spread`, `OMP_PLACES=cores`) because first-touch
  already lands pages on the local node; the gap widens when pinning
  is weak.
- `interleave` should trail both on NUMA-sensitive matrices — that is
  the control showing what happens without per-page binding.
- GPU peak is usually hit at `block=256`, `grid_mult=8–16`; larger
  blocks expose register pressure, smaller grids under-subscribe SMs.
- At `N=8192` the working set fits in Infinity Cache on MI300A;
  measured bandwidth approaches the L2 ceiling. At `N=24576` the
  working set is firmly in HBM territory on both clusters.

## Notes on determinism

All input values are constants (`A = 1.0`, `B = 2.0`, `C = 0`) — this
bench does not use the shared `Xor64Rng` from `common/prng.h` because
the kernel is layout-invariant up to input values and the bandwidth
ceiling is what we're measuring.
