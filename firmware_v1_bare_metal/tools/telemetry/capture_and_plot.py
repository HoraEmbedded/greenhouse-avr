"""One command: runs Wokwi headlessly (wokwi-cli), captures serial
output, turns it into a graph.

Setup: npm install -g wokwi-cli ; set WOKWI_CLI_TOKEN (wokwi.com/dashboard/ci)

Usage:
    python capture_and_plot.py --duration 120
    python capture_and_plot.py --skip-capture --serial-log captured_serial.txt
"""
import argparse
import subprocess
import sys
from pathlib import Path
from typing import Optional

from logger import run
from logger_from_file import FileSource, retimestamp
from plot import build_figure, load_csv


def capture(project_dir: str, serial_log: str, duration_s: float,
            scenario: Optional[str] = None) -> None:
    timeout_ms = int(duration_s * 1000)
    print(f"Starting Wokwi simulation for {duration_s:.0f}s...")

    serial_log = str(Path(serial_log).resolve())
    if scenario:
        scenario = str(Path(scenario).resolve())

    command = ["wokwi-cli", project_dir, "--serial-log-file", serial_log,
               "--timeout", str(timeout_ms)]
    if scenario:
        command += ["--scenario", scenario]

    try:
        result = subprocess.run(command, capture_output=True, text=True)
    except FileNotFoundError:
        raise SystemExit("wokwi-cli not found. Install: npm install -g wokwi-cli")

    print(result.stdout)
    if result.returncode not in (0, 42):
        print(result.stderr, file=sys.stderr)
        raise SystemExit(f"wokwi-cli failed (code {result.returncode})")

    print(f"Captured -> {serial_log}")


def process(serial_log: str, csv_path: str, interval_s: float) -> None:
    with open(serial_log, encoding="utf-8", errors="replace") as f:
        lines = f.readlines()

    source = FileSource(lines)
    with open(csv_path, "w", newline="", encoding="utf-8") as csv_file:
        try:
            run(source, csv_file)
        except StopIteration:
            pass

    retimestamp(csv_path, interval_s)
    print(f"CSV -> {csv_path}")


def plot(csv_path: str, output_path: str) -> None:
    timestamps, temps, soils, fan, pump, sensor_error = load_csv(csv_path)
    if not timestamps:
        raise SystemExit(f"No telemetry rows found in {csv_path}")
    fig = build_figure(timestamps, temps, soils, fan, pump, sensor_error)
    fig.savefig(output_path, dpi=150)
    print(f"Plot -> {output_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-dir", default="../..")
    parser.add_argument("--duration", type=float, default=120)
    parser.add_argument("--interval", type=float, default=2.0)
    parser.add_argument("--serial-log", default="captured_serial.txt")
    parser.add_argument("--csv", default="telemetry.csv")
    parser.add_argument("--output", default="telemetry.png")
    parser.add_argument("--skip-capture", action="store_true")
    parser.add_argument("--scenario", default=None)
    args = parser.parse_args()

    if not args.skip_capture:
        capture(args.project_dir, args.serial_log, args.duration, args.scenario)

    process(args.serial_log, args.csv, args.interval)
    plot(args.csv, args.output)


if __name__ == "__main__":
    main()
