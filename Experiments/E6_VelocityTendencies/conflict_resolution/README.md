# E6 / Conflict Resolution (Task T6.4) — *(TODO)*

Reads per-loopnest rankings produced by `loopnest_{1..6}/` and
resolves cross-nest conflicts by call-frequency weighting
(Config A: 1/6, Config B: 5/6), emitting the globally selected
layouts consumed by `full_velocity_tendencies/`.

## Planned usage

```bash
bash ../../common/setup.sh
source ../../common/activate.sh
python resolve_conflicts.py            # TODO
```

Inputs:  `../loopnest_{1..6}/results/*/*.csv`
Output:  `selected_layouts.json`
