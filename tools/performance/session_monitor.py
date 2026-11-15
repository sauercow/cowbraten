#!/usr/bin/env python3
"""Run Cowbraten and produce a compact long-session performance report."""

from __future__ import annotations

import argparse
import csv
import json
import math
import os
import platform
import re
import shutil
import signal
import statistics
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def read_text(path: Path) -> str | None:
    try:
        return path.read_text(errors="replace").strip()
    except (OSError, PermissionError):
        return None


def read_number(path: Path, scale: float = 1.0) -> float | None:
    value = read_text(path)
    try:
        return float(value) / scale if value is not None else None
    except ValueError:
        return None


def safe_name(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", value.lower()).strip("_")


def parse_duration(value: str) -> float:
    match = re.fullmatch(r"\s*(\d+(?:\.\d+)?)\s*([smh]?)\s*", value)
    if not match:
        raise argparse.ArgumentTypeError("usa segundos o un valor como 30m o 2h")
    number, unit = match.groups()
    return float(number) * {"": 1, "s": 1, "m": 60, "h": 3600}[unit]


def percentile(values: list[float], fraction: float) -> float | None:
    if not values:
        return None
    ordered = sorted(values)
    position = (len(ordered) - 1) * fraction
    lower, upper = math.floor(position), math.ceil(position)
    if lower == upper:
        return ordered[lower]
    return ordered[lower] * (upper - position) + ordered[upper] * (position - lower)


def discover_sensors(hwmon_root: Path = Path("/sys/class/hwmon")) -> dict[str, Path]:
    sensors: dict[str, Path] = {}
    for directory in sorted(hwmon_root.glob("hwmon*")):
        device = safe_name(read_text(directory / "name") or directory.name)
        for source in sorted(directory.glob("temp*_input")):
            index = source.name[4:-6]
            label = read_text(directory / f"temp{index}_label") or f"temp{index}"
            sensors[f"temp_c_{device}_{safe_name(label)}"] = source
        for kind in ("power", "fan"):
            patterns = (f"{kind}*_average", f"{kind}*_input")
            seen: set[str] = set()
            for pattern in patterns:
                for source in sorted(directory.glob(pattern)):
                    index = source.name[len(kind):].split("_", 1)[0]
                    if index in seen:
                        continue
                    seen.add(index)
                    label = read_text(directory / f"{kind}{index}_label") or f"{kind}{index}"
                    unit = "w" if kind == "power" else "rpm"
                    sensors[f"{kind}_{unit}_{device}_{safe_name(label)}"] = source
    return sensors


def discover_gpu_files(drm_root: Path = Path("/sys/class/drm")) -> dict[str, Path]:
    result: dict[str, Path] = {}
    for card in sorted(drm_root.glob("card[0-9]*")):
        device = card / "device"
        for filename, label in (
            ("gpu_busy_percent", "gpu_busy_percent"),
            ("mem_info_vram_used", "gpu_vram_used_mib"),
            ("mem_info_vram_total", "gpu_vram_total_mib"),
        ):
            path = device / filename
            if path.is_file():
                result[f"{label}_{card.name}"] = path
    return result


def process_snapshot(pid: int) -> dict[str, float]:
    stat = read_text(Path(f"/proc/{pid}/stat"))
    if not stat:
        raise ProcessLookupError(pid)
    # The command name may contain spaces and parentheses; fields start after its last ')'.
    fields = stat[stat.rfind(")") + 2:].split()
    ticks = os.sysconf("SC_CLK_TCK")
    page_size = os.sysconf("SC_PAGE_SIZE")
    status: dict[str, str] = {}
    for line in (read_text(Path(f"/proc/{pid}/status")) or "").splitlines():
        if ":" in line:
            key, value = line.split(":", 1)
            status[key] = value.strip()
    io_values: dict[str, float] = {}
    for line in (read_text(Path(f"/proc/{pid}/io")) or "").splitlines():
        if ":" in line:
            key, value = line.split(":", 1)
            try:
                io_values[key] = float(value.strip())
            except ValueError:
                pass

    return {
        "process_cpu_s": (float(fields[11]) + float(fields[12])) / ticks,
        "rss_mib": float(fields[21]) * page_size / (1024 * 1024),
        "threads": float(status.get("Threads", "0")),
        "read_mib": io_values.get("read_bytes", 0.0) / (1024 * 1024),
        "write_mib": io_values.get("write_bytes", 0.0) / (1024 * 1024),
        "voluntary_context_switches": float(
            status.get("voluntary_ctxt_switches", "0")),
        "involuntary_context_switches": float(
            status.get("nonvoluntary_ctxt_switches", "0")),
    }


def system_cpu_snapshot() -> tuple[float, float]:
    line = (read_text(Path("/proc/stat")) or "").splitlines()[0].split()
    values = [float(value) for value in line[1:]]
    idle = values[3] + (values[4] if len(values) > 4 else 0.0)
    return sum(values), idle


def sample_system(sensors: dict[str, Path], gpu_files: dict[str, Path]) -> dict[str, float]:
    result: dict[str, float] = {}
    load = os.getloadavg()
    result.update(load_1m=load[0], load_5m=load[1], load_15m=load[2])

    meminfo: dict[str, float] = {}
    for line in (read_text(Path("/proc/meminfo")) or "").splitlines():
        if ":" in line:
            key, value = line.split(":", 1)
            try:
                meminfo[key] = float(value.split()[0]) / 1024
            except (ValueError, IndexError):
                pass
    result["system_memory_used_mib"] = max(
        0.0, meminfo.get("MemTotal", 0.0) - meminfo.get("MemAvailable", 0.0))

    frequencies = [read_number(path, 1000.0) for path in
                   Path("/sys/devices/system/cpu").glob("cpu[0-9]*/cpufreq/scaling_cur_freq")]
    frequencies = [value for value in frequencies if value is not None]
    if frequencies:
        result["cpu_frequency_mhz_avg"] = statistics.fmean(frequencies)

    for name, path in sensors.items():
        scale = 1000.0 if name.startswith("temp_c_") else (
            1_000_000.0 if name.startswith("power_w_") else 1.0)
        value = read_number(path, scale)
        if value is not None:
            result[name] = value
    for name, path in gpu_files.items():
        scale = 1024 * 1024 if "vram" in name else 1.0
        value = read_number(path, scale)
        if value is not None:
            result[name] = value

    for battery in Path("/sys/class/power_supply").glob("BAT*"):
        prefix = safe_name(battery.name)
        for filename, label, scale in (
            ("capacity", "battery_percent", 1.0),
            ("power_now", "battery_power_w", 1_000_000.0),
        ):
            value = read_number(battery / filename, scale)
            if value is not None:
                result[f"{label}_{prefix}"] = value
    return result


def numeric_rows(path: Path) -> list[dict[str, float]]:
    if not path.is_file():
        return []
    result = []
    with path.open(newline="") as source:
        for row in csv.DictReader(source):
            try:
                result.append({key: float(value) for key, value in row.items()
                               if value not in (None, "")})
            except ValueError:
                continue
    return result


def metric(rows: list[dict[str, float]], key: str) -> list[float]:
    return [row[key] for row in rows if key in row and math.isfinite(row[key])]


def fmt(value: float | None, digits: int = 1) -> str:
    return "N/D" if value is None else f"{value:.{digits}f}"


def generate_report(report_dir: Path, metadata: dict) -> Path:
    system_rows = numeric_rows(report_dir / "system.csv")
    engine_rows = numeric_rows(report_dir / "engine.csv")
    duration = metric(system_rows, "elapsed_s")
    duration_s = max(duration, default=0.0)

    lines = [
        "# Reporte de sesión de Cowbraten",
        "",
        f"- Inicio: {metadata['started_at']}",
        f"- Duración observada: {duration_s / 60:.1f} min",
        f"- Salida del juego: {metadata.get('exit_code', 'N/D')}",
        f"- Sistema: {metadata['platform']}",
        f"- CPU lógicos: {metadata['logical_cpus']}",
        f"- Commit: {metadata.get('git_commit', 'N/D')}",
        "",
        "## Resumen",
        "",
    ]

    stable_engine_rows = engine_rows
    engine_elapsed = metric(engine_rows, "elapsed_s")
    discarded_warmup = bool(engine_elapsed and max(engine_elapsed) >= 10)
    if discarded_warmup:
        stable_engine_rows = [row for row in engine_rows
                              if row.get("elapsed_s", 0) >= 5]
    fps = metric(stable_engine_rows, "fps")
    frame_max = metric(stable_engine_rows, "frame_ms_max")
    long_16 = metric(stable_engine_rows, "frames_over_16ms")
    long_33 = metric(stable_engine_rows, "frames_over_33ms")
    if fps:
        lines += [
            f"- FPS por intervalo: media {fmt(statistics.fmean(fps))}, "
            f"p5 {fmt(percentile(fps, 0.05))}, mínimo {fmt(min(fps))}.",
            f"- Peor frame observado: {fmt(max(frame_max), 2)} ms.",
            f"- Frames >16.7 ms / >33.3 ms: {int(sum(long_16))} / {int(sum(long_33))}.",
        ]
        if discarded_warmup:
            lines.append("- El resumen de FPS omite los primeros 5 s de arranque; "
                         "los datos crudos sí los conservan.")
    else:
        lines.append("- No hubo telemetría interna de FPS (¿se compiló el cliente actualizado?).")

    process_cpu = metric(system_rows, "process_cpu_percent")
    rss = metric(system_rows, "rss_mib")
    gpu_busy_keys = sorted({key for row in system_rows for key in row
                            if key.startswith("gpu_busy_percent_")})
    if process_cpu:
        lines.append(f"- CPU del proceso: media {fmt(statistics.fmean(process_cpu))}%, "
                     f"máxima {fmt(max(process_cpu))}% (100% equivale a un núcleo).")
    if rss:
        lines.append(f"- Memoria residente máxima: {fmt(max(rss))} MiB.")
    for key in gpu_busy_keys:
        values = metric(system_rows, key)
        if values:
            lines.append(f"- Uso de {key.removeprefix('gpu_busy_percent_')}: "
                         f"media {fmt(statistics.fmean(values))}%, máxima {fmt(max(values))}%.")

    temp_keys = sorted({key for row in system_rows for key in row
                        if key.startswith("temp_c_")})
    lines += ["", "## Temperaturas máximas", ""]
    if temp_keys:
        for key in temp_keys:
            values = metric(system_rows, key)
            if values:
                lines.append(f"- {key.removeprefix('temp_c_')}: {fmt(max(values))} °C "
                             f"(media {fmt(statistics.fmean(values))} °C)")
    else:
        lines.append("No se encontraron sensores térmicos legibles en `/sys/class/hwmon`.")

    lines += ["", "## Evolución", ""]
    if len(fps) >= 6:
        third = max(1, len(fps) // 3)
        early, late = statistics.fmean(fps[:third]), statistics.fmean(fps[-third:])
        change = (late - early) * 100.0 / early if early else 0.0
        lines.append(f"- FPS medio del primer tercio: {fmt(early)}; último tercio: "
                     f"{fmt(late)} ({change:+.1f}%).")
        if change < -10:
            lines.append("- Hay una caída sostenida superior al 10%; conviene contrastarla "
                         "con las curvas de temperatura y frecuencia del CSV.")
        else:
            lines.append("- No se observa una caída sostenida de FPS superior al 10% entre "
                         "el primer y el último tercio.")
    else:
        lines.append("La sesión fue demasiado corta para comparar tercios con fiabilidad.")

    lines += [
        "",
        "## Archivos incluidos",
        "",
        "- `system.csv`: proceso, carga, memoria, frecuencias, GPU y sensores.",
        "- `engine.csv`: FPS y frametimes agregados cada segundo.",
        "- `metadata.json`: entorno, comando y versión analizada.",
        "",
        "Para un diagnóstico, comparte esta carpeta completa (preferiblemente comprimida).",
        "",
    ]
    report = report_dir / "report.md"
    report.write_text("\n".join(lines))
    return report


def git_commit() -> str | None:
    try:
        return subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"], cwd=ROOT,
            check=True, text=True, capture_output=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def merge_init_script(arguments: list[str], telemetry_script: str) -> list[str]:
    result, existing = [], None
    for argument in arguments:
        if argument.startswith("-x"):
            existing = argument[2:]
        else:
            result.append(argument)
    combined = f"{existing}; {telemetry_script}" if existing else telemetry_script
    result.append(f"-x{combined}")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Monitor ligero para sesiones largas de Cowbraten")
    parser.add_argument("--output", type=Path, default=ROOT / "session_reports",
                        help="directorio padre de los reportes")
    parser.add_argument("--interval", type=float, default=5.0,
                        help="segundos entre muestras del sistema (por defecto: 5)")
    parser.add_argument("--duration", type=parse_duration,
                        help="duración máxima, por ejemplo 45m o 2h")
    parser.add_argument("--launcher", type=Path, default=ROOT / "cowbraten.sh",
                        help="launcher del cliente")
    parser.add_argument("--game-home", type=Path,
                        default=Path.home() / ".cowbraten",
                        help="directorio usado por -q")
    parser.add_argument("game_args", nargs=argparse.REMAINDER,
                        help="argumentos del juego; sepáralos con --")
    args = parser.parse_args()
    if args.interval < 0.25:
        parser.error("--interval debe ser al menos 0.25 segundos")

    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    game_args = args.game_args[1:] if args.game_args[:1] == ["--"] else args.game_args
    game_home = args.game_home.expanduser()
    for argument in game_args:
        if argument.startswith("-q") and len(argument) > 2:
            game_home = Path(argument[2:]).expanduser()
    report_dir = args.output.expanduser().resolve() / timestamp
    report_dir.mkdir(parents=True, exist_ok=False)
    engine_name = f"session_engine_{timestamp}.csv"
    engine_source = game_home / engine_name
    try:
        engine_source.unlink(missing_ok=True)
    except OSError:
        pass

    game_args = merge_init_script(game_args,
        f'session_capture_start "{engine_name}" 1000')
    command = [str(args.launcher.expanduser().resolve()), *game_args]
    recorded_command = [command[0], *[
        "-x<init script omitted>" if argument.startswith("-x") else argument
        for argument in command[1:]
    ]]
    metadata = {
        "started_at": datetime.now(timezone.utc).astimezone().isoformat(),
        "platform": platform.platform(),
        "python": platform.python_version(),
        "logical_cpus": os.cpu_count(),
        "git_commit": git_commit(),
        "command": recorded_command,
        "sample_interval_s": args.interval,
    }
    (report_dir / "metadata.json").write_text(
        json.dumps(metadata, indent=2, ensure_ascii=False) + "\n")

    sensors = discover_sensors()
    gpu_files = discover_gpu_files()
    initial_system = sample_system(sensors, gpu_files)
    fieldnames = [
        "elapsed_s", "process_cpu_percent", "system_cpu_percent",
        "rss_mib", "threads", "read_mib", "write_mib",
        "voluntary_context_switches", "involuntary_context_switches",
        *sorted(initial_system),
    ]

    print(f"Reporte: {report_dir}")
    print("Iniciando Cowbraten; cierra el juego normalmente al terminar la sesión.")
    process = subprocess.Popen(command, cwd=ROOT)
    started = time.monotonic()
    previous_time = started
    previous_process = process_snapshot(process.pid)
    previous_system_total, previous_system_idle = system_cpu_snapshot()
    exit_code = None

    try:
        with (report_dir / "system.csv").open("w", newline="") as destination:
            writer = csv.DictWriter(destination, fieldnames=fieldnames,
                                    extrasaction="ignore")
            writer.writeheader()
            while process.poll() is None:
                if args.duration and time.monotonic() - started >= args.duration:
                    print("Duración alcanzada; solicitando el cierre del juego.")
                    process.terminate()
                    break
                time.sleep(min(args.interval, 0.5))
                now = time.monotonic()
                if now - previous_time < args.interval:
                    continue
                try:
                    current_process = process_snapshot(process.pid)
                except ProcessLookupError:
                    break
                current_total, current_idle = system_cpu_snapshot()
                elapsed = now - previous_time
                system_delta = current_total - previous_system_total
                row = {
                    "elapsed_s": now - started,
                    "process_cpu_percent": 100.0 * (
                        current_process["process_cpu_s"] -
                        previous_process["process_cpu_s"]) / elapsed,
                    "system_cpu_percent": 100.0 * (1.0 - (
                        current_idle - previous_system_idle) / system_delta)
                        if system_delta > 0 else 0.0,
                    **current_process,
                    **sample_system(sensors, gpu_files),
                }
                writer.writerow(row)
                destination.flush()
                previous_time, previous_process = now, current_process
                previous_system_total, previous_system_idle = current_total, current_idle
            try:
                exit_code = process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                exit_code = process.wait()
    except KeyboardInterrupt:
        print("\nInterrupción recibida; cerrando el monitor.")
        if process.poll() is None:
            process.send_signal(signal.SIGINT)
            try:
                exit_code = process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.terminate()
                exit_code = process.wait(timeout=5)
    finally:
        if exit_code is None:
            exit_code = process.poll()
        metadata["exit_code"] = exit_code
        metadata["finished_at"] = datetime.now(timezone.utc).astimezone().isoformat()
        (report_dir / "metadata.json").write_text(
            json.dumps(metadata, indent=2, ensure_ascii=False) + "\n")
        if engine_source.is_file():
            shutil.copy2(engine_source, report_dir / "engine.csv")
        report = generate_report(report_dir, metadata)
        archive = shutil.make_archive(str(report_dir), "zip", root_dir=report_dir)
        print(f"Reporte generado: {report}")
        print(f"Archivo listo para compartir: {archive}")
    return 0 if exit_code in (None, 0, -signal.SIGTERM, -signal.SIGINT) else exit_code


if __name__ == "__main__":
    sys.exit(main())
