#!/usr/bin/env python
"""E0 — pretty-print NUMA STREAM peaks for the paper's Hardware table.

Reads results/{daint,beverin}/stream_peak_{cpu,gpu}.csv (single-row each)
and prints a markdown table with CPU / GPU peak in GB/s and TB/s.
"""
from __future__ import annotations

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PLATFORMS = ("daint", "beverin")


def read_peak(path: Path) -> tuple[float, float] | None:
    if not path.exists():
        return None
    with path.open() as f:
        row = next(csv.DictReader(f))
    return float(row["bw_gbs"]), float(row["bw_tbs"])


def main() -> None:
    print(f"| Platform | CPU peak | GPU peak |")
    print(f"|---|---|---|")
    for p in PLATFORMS:
        cpu = read_peak(ROOT / "results" / p / "stream_peak_cpu.csv")
        gpu = read_peak(ROOT / "results" / p / "stream_peak_gpu.csv")
        cpu_s = f"{cpu[1]:.3f} TB/s ({cpu[0]:.1f} GB/s)" if cpu else "—"
        gpu_s = f"{gpu[1]:.3f} TB/s ({gpu[0]:.1f} GB/s)" if gpu else "—"
        print(f"| {p:<8} | {cpu_s} | {gpu_s} |")


if __name__ == "__main__":
    main()
