"""Plots telemetry: temperature/soil with fan/pump/sensor-error periods
shaded. SENSOR_ERR rows have no temperature (NaN, not a crash).

Usage: python plot.py --input telemetry.csv --output telemetry.png
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
            raw_temp = row["temperature_c"]
            temps.append(float(raw_temp) if raw_temp else math.nan)
            soils.append(int(row["soil_humidity_percent"]))
            fan.append(row["fan_on"] == "True")
            pump.append(row["pump_on"] == "True")
            sensor_error.append(row.get("sensor_error") == "True")
    return timestamps, temps, soils, fan, pump, sensor_error


def shade_active(ax, timestamps, active_flags, color, label):
    in_span, span_start, labeled = False, None, False
    for t, active in zip(timestamps, active_flags):
        if active and not in_span:
            span_start, in_span = t, True
        elif not active and in_span:
            ax.axvspan(span_start, t, color=color, alpha=0.15, label=label if not labeled else None)
            labeled, in_span = True, False
    if in_span:
        ax.axvspan(span_start, timestamps[-1], color=color, alpha=0.15, label=label if not labeled else None)


def build_figure(timestamps, temps, soils, fan, pump, sensor_error=None):
    if sensor_error is None:
        sensor_error = [False] * len(timestamps)

    fig, (ax_temp, ax_soil) = plt.subplots(2, 1, figsize=(10, 6), sharex=True)

    ax_temp.plot(timestamps, temps, color="tab:red", label="Temperature (C)")
    ax_temp.set_ylabel("C")
    shade_active(ax_temp, timestamps, fan, color="tab:blue", label="Fan ON")
    shade_active(ax_temp, timestamps, sensor_error, color="tab:red", label="Sensor error")
    ax_temp.legend(loc="upper right")

    ax_soil.plot(timestamps, soils, color="tab:brown", label="Soil moisture (%)")
    ax_soil.set_ylabel("%")
    shade_active(ax_soil, timestamps, pump, color="tab:green", label="Pump ON")
    ax_soil.legend(loc="upper right")

    ax_soil.set_xlabel("Time")
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
    print(f"Saved: {args.output}")


if __name__ == "__main__":
    main()
