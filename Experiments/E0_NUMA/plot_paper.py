#!/usr/bin/env python
"""E0 -- refresh the canonical STREAM peak JSON from measured CSVs.

Reads the per-rep CSVs written by E0_NUMA/bench_stream{,_gpu} at
    results/{daint,beverin}/stream_peak_{cpu,gpu}.csv
and, for every (platform, device) pair where we actually have data,
**overwrites** the corresponding entry in ../common/stream_peak.json.
Entries without matching CSV data are left untouched, so missing a
platform never clobbers the hardcoded fallback.

Peak = max(bw_gbs) across every row in the CSV = min-time across reps,
setups, kernels, and flush modes. That matches the legacy
common/bench_stream.cpp peak methodology (min-of-time) and the numbers
quoted in the paper's Hardware table.

Also prints a markdown summary to stdout for quick inspection.

Downstream plot scripts load the JSON via:

    from plot_util import load_stream_peaks
    STREAM_PEAK = load_stream_peaks()

so the same values that flow from this script reach every figure.
"""
from __future__ import annotations

import csv
import json
from pathlib import Path

ROOT       = Path(__file__).resolve().parent
COMMON_DIR = ROOT.parent / "common"
JSON_PATH  = COMMON_DIR / "stream_peak.json"
PLATFORMS  = ("daint", "beverin")


def measured_peak_tbs(csv_path: Path) -> float:
    """Peak bandwidth (TB/s) across every row of a per-rep CSV. Returns 0.0
    if the file is missing, empty, or lacks a `bw_gbs` column."""
    if not csv_path.exists():
        return 0.0
    best = 0.0
    with csv_path.open() as f:
        reader = csv.DictReader(f)
        if "bw_gbs" not in (reader.fieldnames or ()):
            return 0.0
        for row in reader:
            try:
                v = float(row["bw_gbs"])
            except (TypeError, ValueError):
                continue
            if v > best:
                best = v
    return best / 1000.0


def main():
    with JSON_PATH.open() as f:
        doc = json.load(f)
    peaks  = doc["peaks_tbs"]
    labels = doc["platform_labels"]

    # {platform -> {device -> (label, measured_tbs, json_tbs_before)}}
    results: dict[str, dict[str, tuple[str, float, float]]] = {}
    for platform in PLATFORMS:
        by_device: dict[str, tuple[str, float, float]] = {}
        for device, fname in (("cpu", "stream_peak_cpu.csv"),
                              ("gpu", "stream_peak_gpu.csv")):
            label    = labels[platform][device]
            measured = measured_peak_tbs(ROOT / "results" / platform / fname)
            prior    = float(peaks.get(label, 0.0))
            by_device[device] = (label, measured, prior)
            if measured > 0.0:
                peaks[label] = round(measured, 4)
        results[platform] = by_device

    # Persist. Write only when something measured updated the table, to
    # avoid spurious diffs in `git status` when the script runs on a
    # machine without any CSVs.
    any_measured = any(m > 0.0 for plat in results.values()
                                for (_, m, _) in plat.values())
    if any_measured:
        JSON_PATH.write_text(json.dumps(doc, indent=2) + "\n")

    # Human summary.
    print(f"# STREAM peaks (source: {JSON_PATH.relative_to(COMMON_DIR.parent)})")
    print()
    print("| Platform | Device | Label | JSON (TB/s) | Measured (TB/s) | Source |")
    print("|---|---|---|---|---|---|")
    for platform in PLATFORMS:
        for device in ("cpu", "gpu"):
            label, measured, prior = results[platform][device]
            src = "measured" if measured > 0 else "fallback"
            measured_s = f"{measured:.3f}" if measured > 0 else "—"
            final = peaks[label]
            print(f"| {platform} | {device.upper()} | {label} | "
                  f"{final:.3f} | {measured_s} | {src} |")
    print()
    if any_measured:
        print(f"wrote {JSON_PATH}")
    else:
        print("no measured data found; JSON left unchanged")


if __name__ == "__main__":
    main()
