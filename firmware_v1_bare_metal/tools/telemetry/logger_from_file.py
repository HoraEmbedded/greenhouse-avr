"""Builds a telemetry CSV from captured serial text (e.g. copy-pasted
from Wokwi's Serial Monitor), instead of a live port. Re-timestamps
rows evenly by --interval since a one-shot file read has no real timing.

Usage: python logger_from_file.py --input captured_serial.txt --output telemetry.csv
"""
import argparse
import csv
from datetime import datetime, timedelta, timezone

from logger import run, FIELDNAMES


class FileSource:
    """Stands in for a serial port: readline() drains a list, then
    raises StopIteration (a real port would just keep waiting)."""
    def __init__(self, lines):
        self._lines = list(lines)

    def readline(self):
        if not self._lines:
            raise StopIteration
        return self._lines.pop(0)


def retimestamp(csv_path: str, interval_seconds: float) -> None:
    with open(csv_path, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))

    start = datetime.now(timezone.utc)
    for index, row in enumerate(rows):
        row["timestamp_utc"] = (start + timedelta(seconds=index * interval_seconds)).isoformat()

    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=FIELDNAMES)
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", default="telemetry.csv")
    parser.add_argument("--interval", type=float, default=2.0,
                         help="Seconds between readings (matches Timer1)")
    args = parser.parse_args()

    with open(args.input, encoding="utf-8", errors="replace") as f:
        lines = f.readlines()

    source = FileSource(lines)
    with open(args.output, "w", newline="", encoding="utf-8") as csv_file:
        try:
            run(source, csv_file)
        except StopIteration:
            pass

    retimestamp(args.output, args.interval)
    print(f"Done: {args.output}, re-timestamped at {args.interval}s")


if __name__ == "__main__":
    main()
