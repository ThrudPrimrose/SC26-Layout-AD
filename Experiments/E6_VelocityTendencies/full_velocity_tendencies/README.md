# E6 / Full Velocity Tendencies (Task T6.5) — *(lives in R2)*

End-to-end evaluation of the velocity-tendencies module as a DaCe
SDFG. This stage runs against **repository R2
(`spcl/icon-artifacts`, branch `sc26`)** with DaCe branch
`f2dace/staging`.

Layout transformations from `yakup/dev` are cherry-picked into
`f2dace/staging` by `prepare.sh` in R2.

## Planned usage

```bash
# 1. One-time (from repo root) with the right DaCe branch
DACE_BRANCH=f2dace/staging bash ../../common/setup.sh

# 2. Switch the active DaCe branch for this stage
DACE_BRANCH=f2dace/staging source ../../common/activate.sh

# 3. Submit (scripts imported from R2)
sbatch run_daint.sh                    # TODO
sbatch run_beverin.sh                  # TODO

# 4. Plot
python plot_paper.py                   # Figure 13
```

Inputs:  `../conflict_resolution/selected_layouts.json`
Output:  `results/{daint,beverin}/full_module_*.csv`
