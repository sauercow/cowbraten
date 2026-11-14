#!/usr/bin/env python3
import csv
import importlib.util
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "performance_analyze", ROOT / "tools/performance/analyze.py")
ANALYZE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ANALYZE)


def check(condition, message):
    if not condition:
        raise AssertionError(message)
    print(f"PASS  {message}")


check(ANALYZE.percentile([1, 2, 3, 4], 0.5) == 2.5,
      "percentiles use linear interpolation")
check(ANALYZE.low_fps([1000.0] * 99 + [10000.0], 0.01) == 100.0,
      "1% low is the inverse mean of the slowest one percent")

columns = ["frame", "interval_us", "cpu_frame_us", "gpu_game_us"]
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "capture.csv"
    with path.open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=columns)
        writer.writeheader()
        writer.writerow(dict(frame=0, interval_us=999999,
                             cpu_frame_us=999999, gpu_game_us=-1))
        writer.writerow(dict(frame=1, interval_us=1000,
                             cpu_frame_us=500, gpu_game_us=-1))
        writer.writerow(dict(frame=2, interval_us=2000,
                             cpu_frame_us=700, gpu_game_us=400))
    summary = ANALYZE.summarize(path)

check(summary["samples"] == 2 and summary["fps_average"] == 666.6666666666666,
      "summary discards the capture boundary sample")
check(summary["cpu_frame_ms_mean"] == 0.6,
      "microsecond columns are reported in milliseconds")
check(summary["gpu_game_ms_mean"] == 0.4,
      "unavailable asynchronous GPU samples are excluded")

print("\n5 passed, 0 failed")
