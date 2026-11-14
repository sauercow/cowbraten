#!/usr/bin/env python3
import argparse
import csv
import json
import math
import statistics
from pathlib import Path


def percentile(values, fraction):
    ordered = sorted(values)
    if not ordered:
        return None
    position = (len(ordered) - 1) * fraction
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return ordered[lower]
    weight = position - lower
    return ordered[lower] * (1.0 - weight) + ordered[upper] * weight


def low_fps(frame_times_us, fraction):
    count = max(1, math.ceil(len(frame_times_us) * fraction))
    slowest = sorted(frame_times_us, reverse=True)[:count]
    return 1_000_000.0 / statistics.fmean(slowest)


def summarize(path):
    with path.open(newline="") as source:
        rows = list(csv.DictReader(source))
    if len(rows) > 1:
        rows = rows[1:]
    numeric = {
        key: [float(row[key]) for row in rows
              if not (key.startswith("gpu_") and float(row[key]) < 0)]
        for key in rows[0]
        if key != "frame"
    } if rows else {}
    intervals = numeric.get("interval_us", [])
    summary = {
        "file": str(path),
        "samples": len(rows),
        "duration_s": sum(intervals) / 1_000_000.0,
    }
    if intervals:
        summary.update({
            "fps_average": 1_000_000.0 / statistics.fmean(intervals),
            "fps_median": statistics.median(
                [1_000_000.0 / value for value in intervals]),
            "fps_1_percent_low": low_fps(intervals, 0.01),
            "fps_0_1_percent_low": low_fps(intervals, 0.001),
            "frametime_p50_ms": percentile(intervals, 0.50) / 1000.0,
            "frametime_p95_ms": percentile(intervals, 0.95) / 1000.0,
            "frametime_p99_ms": percentile(intervals, 0.99) / 1000.0,
            "frametime_max_ms": max(intervals) / 1000.0,
        })
    for key, values in numeric.items():
        if key == "interval_us" or not values:
            continue
        label = key[:-2] + "ms" if key.endswith("_us") else key
        divisor = 1000.0 if key.endswith("_us") else 1.0
        summary[f"{label}_mean"] = statistics.fmean(values) / divisor
        summary[f"{label}_p95"] = percentile(values, 0.95) / divisor
        summary[f"{label}_p99"] = percentile(values, 0.99) / divisor
        summary[f"{label}_max"] = max(values) / divisor
    return summary


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", nargs="+", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = [summarize(path) for path in args.csv]
    encoded = json.dumps(result, indent=2, sort_keys=True)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded + "\n")
    print(encoded)


if __name__ == "__main__":
    main()
