"""Plots a telemetry CSV produced by logger.py: temperature and soil
moisture over time, with the fan/pump ON periods shaded behind the curve
so the effect of the hysteresis logic is visible directly. Sensor-error
periods (parser.py's SENSOR_ERR rows, no temperature to plot) are shaded
in a third colour instead of being silently dropped or crashing on a
missing value.

Usage:
    python3 plot.py --input telemetry.csv --output telemetry.png

Requires: pip install matplotlib
"""
import argparse
import csv
import math
from datetime import datetime

import matplotlib.pyplot as plt


def load_csv(path):
    timestamps, temps, soils, fan, pump, sensor_error = [], [], [], [], [], []
    with open(path, newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            timestamps.append(datetime.fromisoformat(row["timestamp_utc"]))
            # A SENSOR_ERR row has no temperature (see parser.py) -- the
            # CSV cell is empty in that case. NaN, not a skipped point:
            # it keeps this list the same length as timestamps, and
            # matplotlib draws a gap for NaN instead of raising.
            raw_temp = row["temperature_c"]
            temps.append(float(raw_temp) if raw_temp else math.nan)
            soils.append(int(row["soil_humidity_percent"]))
            fan.append(row["fan_on"] == "True")
            pump.append(row["pump_on"] == "True")
            sensor_error.append(row.get("sensor_error") == "True")
    return timestamps, temps, soils, fan, pump, sensor_error


def shade_active(ax, timestamps, active_flags, color, label):
    """Shades the background wherever active_flags is True, so ON periods
    for the fan/pump (or sensor-error episodes) are visible directly
    behind the sensor curve."""
    in_span = False
    span_start = None
    labeled = False
    for t, active in zip(timestamps, active_flags):
        if active and not in_span:
            span_start = t
            in_span = True
        elif not active and in_span:
            ax.axvspan(span_start, t, color=color, alpha=0.15,
                       label=label if not labeled else None)
            labeled = True
            in_span = False
    if in_span:
        ax.axvspan(span_start, timestamps[-1], color=color, alpha=0.15,
                   label=label if not labeled else None)


def build_figure(timestamps, temps, soils, fan, pump, sensor_error=None):
    """Builds and returns the figure, separated from file I/O so it can be
    exercised by a test without writing a PNG to disk. sensor_error is
    optional and defaults to "no errors" so older callers/CSVs without
    that column still work."""
    if sensor_error is None:
        sensor_error = [False] * len(timestamps)

    fig, (ax_temp, ax_soil) = plt.subplots(2, 1, figsize=(10, 6), sharex=True)

    ax_temp.plot(timestamps, temps, color="tab:red", label="Température (°C)")
    ax_temp.set_ylabel("°C")
    shade_active(ax_temp, timestamps, fan, color="tab:blue", label="Ventilateur ON")
    shade_active(ax_temp, timestamps, sensor_error, color="tab:red",
                 label="Panne capteur (DHT22)")
    ax_temp.legend(loc="upper right")

    ax_soil.plot(timestamps, soils, color="tab:brown", label="Humidité sol (%)")
    ax_soil.set_ylabel("%")
    shade_active(ax_soil, timestamps, pump, color="tab:green", label="Pompe ON")
    ax_soil.legend(loc="upper right")

    ax_soil.set_xlabel("Temps")
    fig.autofmt_xdate()
    fig.tight_layout()
    return fig


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", default="telemetry.csv")
    parser.add_argument("--output", default="telemetry.png")
    args = parser.parse_args()

    timestamps, temps, soils, fan, pump, sensor_error = load_csv(args.input)
    if not timestamps:
        raise SystemExit(f"No rows found in {args.input}")

    fig = build_figure(timestamps, temps, soils, fan, pump, sensor_error)
    fig.savefig(args.output, dpi=150)
    print(f"Graphique enregistré dans {args.output}")


if __name__ == "__main__":
    main()
