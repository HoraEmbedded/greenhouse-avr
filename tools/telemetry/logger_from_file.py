"""Builds a telemetry CSV from a text file of already-captured serial
output -- e.g. text copied out of Wokwi's Serial Monitor and pasted into
a .txt file -- instead of a live port.

Useful while still in simulation: Wokwi's Serial Monitor shows the text
but doesn't expose a real COM port that pyserial can open. This script
sidesteps that by reusing the EXACT SAME read-parse-write loop as
logger.py (see logger.run(), already tested in
test_logger_integration.py) -- only the source of lines changes.

One thing run() gets wrong for this use case: it timestamps every row
with "now" at the moment it's PROCESSED, which for a file read in one
shot means all rows land within milliseconds of each other -- nothing
like the real ~2 s cadence the firmware actually samples at (Timer1,
see main.c). This script re-timestamps the resulting rows afterwards,
evenly spaced by --interval seconds, so the plot's time axis reflects
the real sampling cadence instead of how fast Python read a text file.

Usage:
    python logger_from_file.py --input captured_serial.txt --output telemetry.csv
    python logger_from_file.py --input captured_serial.txt --output telemetry.csv --interval 2.0

How to get captured_serial.txt: in Wokwi's Serial Monitor, select the
output and copy it, then paste it into a plain text file. Run the
simulation for a few minutes first so there's more than one or two
readings to plot.
"""
import argparse
import csv
from datetime import datetime, timedelta, timezone

from logger import run, FIELDNAMES


class FileSource:
    """Stands in for a pyserial object: readline() returns lines from an
    already-read text file one at a time, then raises to signal there is
    no more input -- a real port would just keep waiting, which a
    fixed-size file can't do."""

    def __init__(self, lines):
        self._lines = list(lines)

    def readline(self):
        if not self._lines:
            raise StopIteration
        return self._lines.pop(0)


def retimestamp(csv_path: str, interval_seconds: float) -> None:
    """Rewrites the timestamp column with evenly spaced values, interval
    apart, preserving every other field and the row order exactly."""
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
    parser.add_argument("--input", required=True,
                         help="Text file with the captured serial output "
                              "(one line per reading, as Wokwi printed it)")
    parser.add_argument("--output", default="telemetry.csv")
    parser.add_argument("--interval", type=float, default=2.0,
                         help="Seconds between readings, matching the "
                              "firmware's real sampling cadence (default: "
                              "2.0, Timer1's actual period in main.c)")
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
    print(f"Terminé : télémétrie écrite dans {args.output}, "
          f"réhorodatée à {args.interval} s d'intervalle")


if __name__ == "__main__":
    main()

