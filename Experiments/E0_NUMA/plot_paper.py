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
import re
from pathlib import Path

ROOT       = Path(__file__).resolve().parent
EXP_ROOT   = ROOT.parent
COMMON_DIR = EXP_ROOT / "common"
JSON_PATH  = COMMON_DIR / "stream_peak.json"
PLATFORMS  = ("daint", "beverin")

# Experiments that report achieved bandwidth in their CSV / stdout
# outputs. The peak we publish for each (platform, device) is the
# maximum observed across all of these -- that way a layout-optimised
# kernel that beats the raw STREAM number doesn't get "above 100%" %-of-peak
# annotations downstream.
PEAK_SOURCE_EXPERIMENTS = (
    "E0_NUMA",
    "E1_MatrixAdd",
    "E2_Conjugation",
    "E3_Transpose",
)

# Column names that any of E0..E3 use for "achieved bandwidth in GB/s".
# E0 / E1 emit `bw_gbs`; E2 emits `gbps`; E3 emits `med_gbs` / `max_gbs`
# (we take whichever is largest; the absolute max across columns and
# rows is what feeds the peak).
_BW_GBS_COLUMNS = ("bw_gbs", "gbps", "max_gbs", "med_gbs")

# Header that separates the CPU and GPU portions of E0_stream_<plat>_<job>.out.
# Lines up to (and including) this header belong to the CPU benchmark; lines
# after it are GPU. Used by ``_peak_from_out_file`` so platforms that only
# have stdout copied locally (no CSVs) can still drive the JSON refresh.
_GPU_SECTION_HEADER = "NUMA STREAM peak GPU"

# Bandwidth tokens in the .out file always end in "<float> GB/s".
_GBS_RE = re.compile(r"(\d+(?:\.\d+)?)\s*GB/s")


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


def _peak_from_e0_out_file(results_dir: Path, device: str) -> float:
    """Fallback peak (TB/s) parsed from the latest ``E0_stream_<plat>_*.out``.

    The bench prints a human-readable trail with `<bw> GB/s` tokens on every
    measured line. The CPU section runs first; the GPU section starts at the
    line containing ``NUMA STREAM peak GPU``. We split on that header, scan
    only the relevant half, and return the max GB/s observed (then convert
    to TB/s). Returns 0.0 if no .out files exist or no GB/s line is found.

    This makes the JSON refresh tolerant of cluster runs where only stdout
    was synced locally and the CSV stayed on the scratch FS.
    """
    if not results_dir.is_dir():
        return 0.0
    out_files = sorted(results_dir.glob("E0_stream_*.out"))
    if not out_files:
        return 0.0
    text = out_files[-1].read_text(errors="replace")
    if _GPU_SECTION_HEADER in text:
        cpu_part, gpu_part = text.split(_GPU_SECTION_HEADER, 1)
    else:
        cpu_part, gpu_part = text, ""
    section = gpu_part if device == "gpu" else cpu_part
    best = 0.0
    for m in _GBS_RE.finditer(section):
        try:
            v = float(m.group(1))
        except ValueError:
            continue
        if v > best:
            best = v
    return best / 1000.0


def _csv_max_gbs(csv_path: Path) -> float:
    """Max bandwidth (GB/s) across any of the recognised columns
    (``bw_gbs`` / ``gbps`` / ``max_gbs`` / ``med_gbs``) in a CSV."""
    if not csv_path.exists():
        return 0.0
    best = 0.0
    try:
        with csv_path.open() as f:
            reader = csv.DictReader(f)
            cols = [c for c in (reader.fieldnames or ()) if c in _BW_GBS_COLUMNS]
            if not cols:
                return 0.0
            for row in reader:
                for c in cols:
                    try:
                        v = float(row[c])
                    except (TypeError, ValueError, KeyError):
                        continue
                    if v > best:
                        best = v
    except OSError:
        return 0.0
    return best


def _is_for_device(filename: str, device: str) -> bool:
    """Filename-based device classification used when scanning E1..E3
    result dirs. ``"_cpu"`` / ``"_gpu"`` substrings (lower- or
    upper-case) are the convention every experiment follows."""
    fn = filename.lower()
    if device == "cpu":
        return "_cpu" in fn or fn.endswith("cpu.csv") or "cpu_" in fn
    return "_gpu" in fn or fn.endswith("gpu.csv") or "gpu_" in fn


def _peak_across_experiments(platform: str, device: str) -> tuple[float, str]:
    """Walk every ``Experiments/<exp>/results/<platform>/`` for the four
    bandwidth-reporting experiments and return ``(peak_tbs, source_tag)``.

    Source tag is ``"<exp>"`` of whichever experiment produced the
    winner (e.g. ``"E1_MatrixAdd"``). Returns ``(0.0, "missing")`` if
    nothing was found.
    """
    best = 0.0
    best_src = "missing"
    for exp in PEAK_SOURCE_EXPERIMENTS:
        results_dir = EXP_ROOT / exp / "results" / platform
        if not results_dir.is_dir():
            continue
        for csv_path in sorted(results_dir.glob("*.csv")):
            if not _is_for_device(csv_path.name, device):
                continue
            v = _csv_max_gbs(csv_path)
            if v > best:
                best = v
                best_src = exp
        # E0's stdout fallback is the only one with a well-defined
        # CPU/GPU split header; treat it as a per-experiment fallback.
        if exp == "E0_NUMA":
            v_tbs = _peak_from_e0_out_file(results_dir, device)
            if v_tbs * 1000.0 > best:
                best = v_tbs * 1000.0
                best_src = f"{exp}.out"
    return best / 1000.0, best_src


def main():
    with JSON_PATH.open() as f:
        doc = json.load(f)
    peaks  = doc["peaks_tbs"]
    labels = doc["platform_labels"]

    # {platform -> {device -> (label, measured_tbs, json_tbs_before, source)}}
    # The peak is the absolute maximum bandwidth observed across E0..E3 for
    # this (platform, device). Taking the max across all experiments rather
    # than just E0 means a layout-optimised kernel that beats raw STREAM
    # (e.g. the blocked transpose in E3 on Grace) does not produce >100%
    # %-of-peak annotations downstream.
    results: dict[str, dict[str, tuple[str, float, float, str]]] = {}
    for platform in PLATFORMS:
        by_device: dict[str, tuple[str, float, float, str]] = {}
        for device in ("cpu", "gpu"):
            label = labels[platform][device]
            measured, source = _peak_across_experiments(platform, device)
            prior = float(peaks.get(label, 0.0))
            by_device[device] = (label, measured, prior, source)
            if measured > 0.0:
                peaks[label] = round(measured, 4)
        results[platform] = by_device

    # Persist. Write only when something measured updated the table, to
    # avoid spurious diffs in `git status` when the script runs on a
    # machine without any CSVs / .out files.
    any_measured = any(m > 0.0 for plat in results.values()
                                for (_, m, _, _) in plat.values())
    if any_measured:
        JSON_PATH.write_text(json.dumps(doc, indent=2) + "\n")

    # Human summary.
    print(f"# STREAM peaks (source: {JSON_PATH.relative_to(COMMON_DIR.parent)})")
    print()
    print("| Platform | Device | Label | JSON (TB/s) | Measured (TB/s) | Source |")
    print("|---|---|---|---|---|---|")
    for platform in PLATFORMS:
        for device in ("cpu", "gpu"):
            label, measured, prior, src = results[platform][device]
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
