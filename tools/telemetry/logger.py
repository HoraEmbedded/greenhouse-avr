"""Reads live telemetry over serial, appends valid readings to a CSV.

Usage: python logger.py --port COM5 --output telemetry.csv
Requires: pip install pyserial
"""
import argparse
import csv
from datetime import datetime, timezone
from pathlib import Path

import serial

from parser import parse_line

FIELDNAMES = [
    "timestamp_utc", "temperature_c", "air_humidity_percent",
    "soil_humidity_percent", "fan_on", "pump_on", "water_ok", "hour",
    "sensor_error", "consecutive_failures",
]


def run(ser, csv_file, verbose: bool = True) -> None:
    """Read-parse-write loop, factored out for testing with a fake port."""
    writer = csv.DictWriter(csv_file, fieldnames=FIELDNAMES)
    if csv_file.tell() == 0:
        writer.writeheader()

    while True:
        raw = ser.readline()
        if not raw:
            continue

        if isinstance(raw, bytes):
            raw = raw.decode("utf-8", errors="replace")

        reading = parse_line(raw)
        if reading is None:
            continue

        row = {"timestamp_utc": datetime.now(timezone.utc).isoformat(), **reading}
        writer.writerow(row)
        csv_file.flush()

        if verbose:
            if reading["sensor_error"]:
                print(f"  ! SENSOR_ERR (x{reading['consecutive_failures']})  "
                      f"Soil={reading['soil_humidity_percent']:>3}%  "
                      f"Fan={'ON ' if reading['fan_on'] else 'OFF'}  "
                      f"Pump={'ON' if reading['pump_on'] else 'OFF'}  "
                      f"Water={'OK ' if reading['water_ok'] else 'LOW'}  "
                      f"Hour={reading['hour']:>2}")
            else:
                print(f"  T={reading['temperature_c']:>3}C  "
                      f"Air={reading['air_humidity_percent']:>3}%  "
                      f"Soil={reading['soil_humidity_percent']:>3}%  "
                      f"Fan={'ON ' if reading['fan_on'] else 'OFF'}  "
                      f"Pump={'ON' if reading['pump_on'] else 'OFF'}  "
                      f"Water={'OK ' if reading['water_ok'] else 'LOW'}  "
                      f"Hour={reading['hour']:>2}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=9600)
    parser.add_argument("--output", default="telemetry.csv")
    args = parser.parse_args()

    output_path = Path(args.output)
    print(f"Opening {args.port} at {args.baud} baud...")
    with serial.Serial(args.port, args.baud, timeout=5) as ser, \
         open(output_path, "a", newline="", encoding="utf-8") as csv_file:
        print(f"Logging to {output_path}. Ctrl+C to stop.\n")
        try:
            run(ser, csv_file)
        except KeyboardInterrupt:
            print("\nStopped.")


if __name__ == "__main__":
    main()
