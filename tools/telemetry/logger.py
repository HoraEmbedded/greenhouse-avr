"""Reads live telemetry from the greenhouse firmware over serial and
appends each valid reading to a CSV file.

Usage:
    python3 logger.py --port /dev/ttyUSB0 --output telemetry.csv
    python3 logger.py --port COM5 --output telemetry.csv     (Windows)

Requires: pip install pyserial

This file is intentionally thin: the only logic it contains is "open a
port, open a file, loop". The actual decision of what a line means lives
in parser.py and is tested there, without any serial port involved.
"""
import argparse
import csv
from datetime import datetime, timezone
from pathlib import Path

import serial

from parser import parse_line

FIELDNAMES = [
    "timestamp_utc", "temperature_c", "air_humidity_percent",
    "soil_humidity_percent", "fan_on", "pump_on",
]


def run(ser, csv_file, verbose: bool = True) -> None:
    """The actual read-parse-write loop, factored out of main() so it can
    be driven by a fake serial-like object in tests (see
    test_logger_integration.py) without touching a real port."""
    writer = csv.DictWriter(csv_file, fieldnames=FIELDNAMES)
    if csv_file.tell() == 0:
        writer.writeheader()

    while True:
        raw = ser.readline()
        if not raw:
            continue  # read timeout with no data: keep waiting

        if isinstance(raw, bytes):
            raw = raw.decode("utf-8", errors="replace")

        reading = parse_line(raw)
        if reading is None:
            # Not every line is telemetry (command responses, the boot
            # message) -- expected, not an error.
            continue

        row = {"timestamp_utc": datetime.now(timezone.utc).isoformat(), **reading}
        writer.writerow(row)
        csv_file.flush()

        if verbose:
            print(f"  T={reading['temperature_c']:>3}C  "
                  f"Air={reading['air_humidity_percent']:>3}%  "
                  f"Soil={reading['soil_humidity_percent']:>3}%  "
                  f"Fan={'ON ' if reading['fan_on'] else 'OFF'}  "
                  f"Pump={'ON' if reading['pump_on'] else 'OFF'}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True,
                         help="Serial port, e.g. COM5 or /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=9600)
    parser.add_argument("--output", default="telemetry.csv",
                         help="CSV file to append to (created if absent)")
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
