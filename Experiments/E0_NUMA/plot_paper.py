#!/usr/bin/env python
"""E0 -- pretty-print NUMA STREAM peaks for the paper's Hardware table.

Per-rep CSVs from bench_stream / bench_stream_gpu are grouped by
(device, setup, kernel, flush) and aggregated by the *minimum* time
(equivalently the *maximum* bandwidth) across reps. This is the "peak"
methodology the paper's Hardware table and the legacy
common/bench_stream.cpp both report.

CPU CSV schema (from E0_NUMA/bench_stream.cpp):
    setup,variant,N,rep,threads,time_s,bw_gbs,checksum,status,flush
GPU CSV schema (from E0_NUMA/bench_stream_gpu.cu):
    kernel,BX,BY,TX,TY,N,rep,time_ms,bw_gbs,checksum,status,flush
"""
from __future__ import annotations

import csv
from pathlib import Path
from collections import defaultdict

ROOT = Path(__file__).resolve().parent
PLATFORMS = ("daint", "beverin")


def peak_bw_by_group(path: Path,
                     group_keys: tuple[str, ...]) -> dict[tuple, float]:
    """Return {(group_key..., flush): peak bw_gbs} from a per-rep CSV.

    Peak = max(bw_gbs) across reps = 1 / min(time) * bytes. Per the paper's
    Hardware table methodology.
    """
    if not path.exists():
        return {}
    best: dict[tuple, float] = defaultdict(lambda: 0.0)
    with path.open() as f:
        for row in csv.DictReader(f):
            key = tuple(row[k] for k in group_keys) + (row["flush"],)
            bw = float(row["bw_gbs"])
            if bw > best[key]:
                best[key] = bw
    return dict(best)


def best_across_configs(groups: dict[tuple, float],
                        flush: str) -> tuple[tuple | None, float]:
    """Pick the highest-bw config for a given flush state."""
    best_key, best_bw = None, 0.0
    for key, bw in groups.items():
        if key[-1] != flush:
            continue
        if bw > best_bw:
            best_key, best_bw = key, bw
    return best_key, best_bw


def fmt(gbs: float) -> str:
    if gbs <= 0:
        return "—"
    return f"{gbs / 1000.0:.3f} TB/s ({gbs:.1f} GB/s)"


def describe(key: tuple | None, keynames: tuple[str, ...]) -> str:
    if key is None:
        return "—"
    # drop trailing flush flag
    return ", ".join(f"{n}={v}" for n, v in zip(keynames, key[:-1]))


def main():
    cpu_keys = ("setup", "variant")
    gpu_keys = ("kernel",)
    print("# E0 NUMA STREAM peaks (peak = min-time across reps)")
    print()
    for p in PLATFORMS:
        print(f"## {p}")
        print()
        cpu = peak_bw_by_group(ROOT / "results" / p / "stream_peak_cpu.csv", cpu_keys)
        gpu = peak_bw_by_group(ROOT / "results" / p / "stream_peak_gpu.csv", gpu_keys)
        print("| Device | flush | Peak | Best config |")
        print("|---|---|---|---|")
        for flush in ("yes", "no"):
            k, bw = best_across_configs(cpu, flush)
            print(f"| CPU | {flush} | {fmt(bw)} | {describe(k, cpu_keys)} |")
        for flush in ("yes", "no"):
            k, bw = best_across_configs(gpu, flush)
            print(f"| GPU | {flush} | {fmt(bw)} | {describe(k, gpu_keys)} |")
        print()


if __name__ == "__main__":
    main()
