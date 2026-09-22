"""Plots V2 telemetry CSV: temperature + soil with fan/pump/fault shading.

Usage:
    python plot_telemetry.py --input telemetry.csv --output telemetry.png
Requires: pip install matplotlib
"""
import argparse
import csv

import matplotlib.pyplot as plt


def load_csv(path):
    ts, temp, hum, soil, fan, pump, water, day, fault, seq = ([] for _ in range(10))
    with open(path, newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            ts.append(int(row["ts"]) / 1000.0)
            temp.append(float(row["temp_c"]))
            hum.append(float(row["hum_pct"]))
            soil.append(float(row["soil_pct"]))
            fan.append(row["fan"] == "true")
            pump.append(row["pump"] == "true")
            water.append(row["water_ok"] == "true")
            day.append(row["day"] == "true")
            fault.append(row["fault"] == "true")
            seq.append(int(row["seq"]))
    return ts, temp, hum, soil, fan, pump, water, day, fault, seq


def shade(ax, ts, flags, color, label):
    in_span, start, labeled = False, None, False
    for t, active in zip(ts, flags):
        if active and not in_span:
            start, in_span = t, True
        elif not active and in_span:
            ax.axvspan(start, t, color=color, alpha=0.15,
                       label=label if not labeled else None)
            labeled, in_span = True, False
    if in_span:
        ax.axvspan(start, ts[-1], color=color, alpha=0.15,
                   label=label if not labeled else None)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    ts, temp, hum, soil, fan, pump, water, day, fault, _ = load_csv(args.input)

    fig, (ax_t, ax_s) = plt.subplots(2, 1, figsize=(10, 6), sharex=True)

    ax_t.plot(ts, temp, color="tab:red", label="Temperature (C)")
    ax_t.plot(ts, hum,  color="tab:orange", label="Air humidity (%)")
    shade(ax_t, ts, fan,   "tab:blue", "Fan ON")
    shade(ax_t, ts, fault, "tab:red",  "Sensor fault")
    ax_t.set_ylabel("Value")
    ax_t.legend(loc="upper right")

    ax_s.plot(ts, soil, color="tab:brown", label="Soil moisture (%)")
    shade(ax_s, ts, pump,  "tab:green", "Pump ON")
    shade(ax_s, ts, [not w for w in water], "tab:gray", "Water LOW")
    ax_s.set_ylabel("%")
    ax_s.set_xlabel("Time (s)")
    ax_s.legend(loc="upper right")

    fig.tight_layout()
    fig.savefig(args.output, dpi=150)
    print(f"Plot -> {args.output}")


if __name__ == "__main__":
    main()