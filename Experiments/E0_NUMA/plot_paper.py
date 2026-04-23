#!/usr/bin/env python
"""E0 -- pretty-print NUMA STREAM peaks for the paper's Hardware table.

Reads the per-rep CSVs written by bench_stream{,_gpu} (schema:
    ...,rep,...,bw_gbs,...,flush
) from results/{daint,beverin}/stream_peak_{cpu,gpu}.csv, aggregates each
(variant, TILE_Y/BX..., flush) group by arithmetic mean bw_gbs, and
reports the single peak configuration for each (device, flush) pair.

The two flush columns correspond to the two methodologies the bench
runs back-to-back:
  * flush=yes -- canonical 8192^2 Jacobi between every timed rep;
                 matches the E1-E6 cold-cache bench methodology.
  * flush=no  -- no flush; successive reps see a warm cache. Matches
                 the legacy common/bench_stream*.c{pp,u} peak numbers.
"""
from __future__ import annotations

import csv
from pathlib import Path
from collections import defaultdict

ROOT = Path(__file__).resolve().parent
PLATFORMS = ("daint", "beverin")


def peak_by_flush(path: Path, group_keys: tuple[str, ...]) -> dict[str, float]:
    """Return {flush: best_mean_bw_gbs} from a per-rep CSV."""
    if not path.exists():
        return {}
    totals: dict[tuple, tuple[float, int]] = defaultdict(lambda: (0.0, 0))
    with path.open() as f:
        for row in csv.DictReader(f):
            key = tuple(row[k] for k in group_keys) + (row["flush"],)
            s, n = totals[key]
            totals[key] = (s + float(row["bw_gbs"]), n + 1)
    peaks: dict[str, float] = {}
    for key, (s, n) in totals.items():
        flush = key[-1]
        mean = s / n
        if mean > peaks.get(flush, 0.0):
            peaks[flush] = mean
    return peaks


def fmt(gbs: float | None) -> str:
    if gbs is None:
        return "—"
    return f"{gbs / 1000.0:.3f} TB/s ({gbs:.1f} GB/s)"


def main():
    cpu_keys = ("variant", "TILE_Y", "TILE_X")
    gpu_keys = ("kernel",)
    print("| Platform | Device | flush=yes (cold) | flush=no (warm) |")
    print("|---|---|---|---|")
    for p in PLATFORMS:
        cpu = peak_by_flush(ROOT / "results" / p / "stream_peak_cpu.csv", cpu_keys)
        gpu = peak_by_flush(ROOT / "results" / p / "stream_peak_gpu.csv", gpu_keys)
        print(f"| {p:<8} | CPU | {fmt(cpu.get('yes'))} | {fmt(cpu.get('no'))} |")
        print(f"| {p:<8} | GPU | {fmt(gpu.get('yes'))} | {fmt(gpu.get('no'))} |")


if __name__ == "__main__":
    main()
