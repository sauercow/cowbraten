#!/usr/bin/env python3
import csv
import importlib.util
import json
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "session_monitor", ROOT / "tools/performance/session_monitor.py")
MONITOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MONITOR)


def check(condition, message):
    if not condition:
        raise AssertionError(message)
    print(f"PASS  {message}")


check(MONITOR.parse_duration("2h") == 7200 and
      MONITOR.parse_duration("1.5m") == 90,
      "duration parser accepts hours and fractional minutes")
check(MONITOR.percentile([1, 2, 3, 4], 0.5) == 2.5,
      "percentiles use linear interpolation")
check(MONITOR.merge_init_script(["-w800", "-xshowfps 1"], "capture") ==
      ["-w800", "-xshowfps 1; capture"],
      "telemetry is appended to an existing init script")

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    hwmon = root / "hwmon0"
    hwmon.mkdir()
    (hwmon / "name").write_text("k10temp\n")
    (hwmon / "temp1_label").write_text("Tctl\n")
    (hwmon / "temp1_input").write_text("75500\n")
    sensors = MONITOR.discover_sensors(root)
    check(list(sensors) == ["temp_c_k10temp_tctl"],
          "hwmon sensors receive stable descriptive names")

with tempfile.TemporaryDirectory() as directory:
    report_dir = Path(directory)
    with (report_dir / "system.csv").open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=[
            "elapsed_s", "process_cpu_percent", "rss_mib",
            "temp_c_k10temp_tctl"])
        writer.writeheader()
        for second in range(1, 7):
            writer.writerow({"elapsed_s": second, "process_cpu_percent": 80,
                             "rss_mib": 300 + second,
                             "temp_c_k10temp_tctl": 60 + second})
    with (report_dir / "engine.csv").open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=[
            "fps", "frame_ms_max", "frames_over_16ms", "frames_over_33ms"])
        writer.writeheader()
        for fps in (120, 118, 119, 110, 108, 109):
            writer.writerow({"fps": fps, "frame_ms_max": 20,
                             "frames_over_16ms": 1, "frames_over_33ms": 0})
    metadata = {"started_at": "test", "platform": "test",
                "logical_cpus": 4, "git_commit": "abc", "exit_code": 0}
    report = MONITOR.generate_report(report_dir, metadata).read_text()
    check("FPS por intervalo" in report and "66.0 °C" in report,
          "report combines engine and thermal summaries")

print("\n5 passed, 0 failed")
