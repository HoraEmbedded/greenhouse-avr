"""End-to-end: generate scenario, capture Wokwi serial, parse, plot.

Usage:
    python run_scenario.py --duration 120
"""
import argparse
import subprocess
import sys
from pathlib import Path


def run(cmd, **kw):
    print(f"$ {' '.join(str(c) for c in cmd)}")
    result = subprocess.run(cmd, **kw)
    if result.returncode not in (0, 42):
        sys.exit(f"Command failed: {cmd}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--duration", type=int, default=120)
    parser.add_argument("--scenario", default="drift_scenario.yaml")
    parser.add_argument("--serial", default="captured_serial.txt")
    parser.add_argument("--csv", default="telemetry.csv")
    parser.add_argument("--plot", default="telemetry.png")
    args = parser.parse_args()

    here = Path(__file__).parent.resolve()
    project_dir = (here / "..").resolve()
    scenarios_dir = here / "scenarios"
    telemetry_dir = here / "telemetry"

    scenario_path = scenarios_dir / args.scenario
    serial_path = here / args.serial
    csv_path = here / args.csv
    plot_path = here / args.plot

    run([sys.executable, str(scenarios_dir / "generate_drift_scenario.py"),
         "--output", str(scenario_path)])

    run(["wokwi-cli", str(project_dir),
         "--serial-log-file", str(serial_path),
         "--scenario", str(scenario_path),
         "--timeout", str(args.duration * 1000)])

    run([sys.executable, str(telemetry_dir / "parse_telemetry.py"),
         "--input", str(serial_path), "--output", str(csv_path)])

    run([sys.executable, str(telemetry_dir / "plot_telemetry.py"),
         "--input", str(csv_path), "--output", str(plot_path)])

    print(f"\nDone. Plot: {plot_path}")


if __name__ == "__main__":
    main()