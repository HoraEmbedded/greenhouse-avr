"""End-to-end: generate scenario, capture Wokwi serial, parse, plot.

Usage:
    python run_scenario.py --duration 120
"""
import argparse
import subprocess
import sys
from pathlib import Path


def run(cmd, **kw):
    print(f"$ {' '.join(cmd)}")
    result = subprocess.run(cmd, **kw)
    if result.returncode not in (0, 42):
        sys.exit(f"Command failed: {cmd}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-dir", default="../..")
    parser.add_argument("--duration", type=int, default=120)
    parser.add_argument("--scenario", default="drift_scenario.yaml")
    parser.add_argument("--serial", default="captured_serial.txt")
    parser.add_argument("--csv", default="telemetry.csv")
    parser.add_argument("--plot", default="telemetry.png")
    args = parser.parse_args()

    here = Path(__file__).parent
    scenario_dir = here / "scenarios"
    telemetry_dir = here / "telemetry"

    run([sys.executable, str(scenario_dir / "generate_drift_scenario.py"),
         "--output", str(scenario_dir / args.scenario)])

    run(["wokwi-cli", args.project_dir,
         "--serial-log-file", args.serial,
         "--scenario", str(scenario_dir / args.scenario),
         "--timeout", str(args.duration * 1000)])

    run([sys.executable, str(telemetry_dir / "parse_telemetry.py"),
         "--input", args.serial, "--output", args.csv])

    run([sys.executable, str(telemetry_dir / "plot_telemetry.py"),
         "--input", args.csv, "--output", args.plot])

    print(f"\nDone. Plot: {args.plot}")


if __name__ == "__main__":
    main()