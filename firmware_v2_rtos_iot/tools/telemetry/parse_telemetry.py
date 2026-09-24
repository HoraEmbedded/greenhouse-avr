"""Parses [TELEMETRY] {...} lines from a Wokwi serial capture into a CSV.

Usage:
    python parse_telemetry.py --input captured_serial.txt --output telemetry.csv
"""
import argparse
import csv
import json
import re

TELEMETRY_RE = re.compile(r"\[TELEMETRY\]\s+(\{.*\})")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    rows = []
    with open(args.input, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = TELEMETRY_RE.search(line)
            if not m:
                continue
            try:
                payload = json.loads(m.group(1))
            except json.JSONDecodeError:
                continue
            rows.append(payload)

    if not rows:
        raise SystemExit("No telemetry lines found.")

    with open(args.output, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)

    print(f"Parsed {len(rows)} telemetry records -> {args.output}")


if __name__ == "__main__":
    main()