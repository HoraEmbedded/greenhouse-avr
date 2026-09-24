"""Generates a Wokwi Automation Scenario: temperature/soil drift
automatically past both thresholds, hold, then drift back -- no manual
interaction needed.

Usage:
    python generate_drift_scenario.py --output drift_scenario.yaml
    python capture_and_plot.py --scenario drift_scenario.yaml --duration 180
"""
import argparse
import yaml


def build_steps(start_temp, peak_temp, start_soil, trough_soil,
                 step_ms, n_up, n_hold, n_down, air_humidity) -> list:
    steps = []

    def set_env(temp, soil_pct):
        position = max(0.0, min(1.0, soil_pct / 100.0))
        steps.append({"set-control": {"part-id": "dht", "control": "temperature", "value": round(temp, 1)}})
        steps.append({"set-control": {"part-id": "dht", "control": "humidity", "value": air_humidity}})
        steps.append({"set-control": {"part-id": "pot", "control": "position", "value": round(position, 4)}})
        steps.append({"delay": f"{step_ms}ms"})

    for i in range(n_up + 1):
        frac = i / n_up
        set_env(start_temp + frac * (peak_temp - start_temp),
                start_soil + frac * (trough_soil - start_soil))

    for _ in range(n_hold):
        set_env(peak_temp, trough_soil)

    for i in range(n_down + 1):
        frac = i / n_down
        set_env(peak_temp - frac * (peak_temp - start_temp),
                trough_soil + frac * (start_soil - trough_soil))

    return steps


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", default="drift_scenario.yaml")
    parser.add_argument("--start-temp", type=float, default=22.0)
    parser.add_argument("--peak-temp", type=float, default=28.0)
    parser.add_argument("--start-soil", type=float, default=70.0)
    parser.add_argument("--trough-soil", type=float, default=15.0)
    parser.add_argument("--air-humidity", type=float, default=55.0)
    parser.add_argument("--step-ms", type=int, default=2000)
    parser.add_argument("--n-up", type=int, default=20)
    parser.add_argument("--n-hold", type=int, default=5)
    parser.add_argument("--n-down", type=int, default=20)
    args = parser.parse_args()

    steps = build_steps(args.start_temp, args.peak_temp, args.start_soil,
                         args.trough_soil, args.step_ms, args.n_up,
                         args.n_hold, args.n_down, args.air_humidity)

    scenario = {"name": "Automated environmental drift", "version": 1, "steps": steps}
    with open(args.output, "w", encoding="utf-8") as f:
        yaml.dump(scenario, f, sort_keys=False, allow_unicode=True)

    n_points = args.n_up + args.n_hold + args.n_down + 2
    total_s = n_points * args.step_ms / 1000
    print(f"Scenario written -> {args.output}")
    print(f"  {n_points} points, ~{total_s:.0f}s simulated time")
    print(f"  Temp: {args.start_temp}C -> {args.peak_temp}C -> {args.start_temp}C")
    print(f"  Soil: {args.start_soil}% -> {args.trough_soil}% -> {args.start_soil}%")
    print(f"  Suggested --timeout: {int(total_s*1000)+5000}")


if __name__ == "__main__":
    main()
