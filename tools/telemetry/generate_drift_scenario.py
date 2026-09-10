"""Generates a Wokwi Automation Scenario (YAML) that scripts a slow,
fully automatic environmental drift: temperature climbs and soil dries
out gradually until each crosses its hysteresis threshold, triggering
the fan and pump on their own -- then drifts back the other way so the
whole ON/OFF cycle is visible in one recording, without touching the
simulation by hand.

This relies on Wokwi's documented Automation Scenarios feature
(set-control on the dht22's "temperature"/"humidity" and the
potentiometer's "position"), which I verified against Wokwi's own docs
(docs.wokwi.com/wokwi-ci/automation-scenarios) rather than assuming from
memory -- but I have no Wokwi account or firmware to run in the
environment I'm writing this in, so the actual wokwi-cli run against
your project is the one thing I could not execute myself.

Run:
    python generate_drift_scenario.py --output drift_scenario.yaml
    wokwi-cli ../.. --scenario drift_scenario.yaml \\
        --serial-log-file captured_serial.txt --timeout <ms>

    # Or, chained straight into a graph with capture_and_plot.py:
    python capture_and_plot.py --scenario drift_scenario.yaml --duration 180
"""
import argparse

import yaml


def build_steps(start_temp: float, peak_temp: float,
                 start_soil_pct: float, trough_soil_pct: float,
                 step_ms: int, n_up: int, n_hold: int, n_down: int,
                 fixed_air_humidity: float) -> list:
    steps = []

    def set_env(temp: float, soil_pct: float) -> None:
        # Wokwi's potentiometer "position" is 0.0-1.0 and maps linearly to
        # the ADC reading (0-1023) -- exactly what soil.c's DEFAULT
        # calibration assumes (0 -> 0%, 1023 -> 100%). If soil.h's
        # calibration constants are ever changed, this mapping needs to
        # change with them.
        position = max(0.0, min(1.0, soil_pct / 100.0))
        steps.append({"set-control": {"part-id": "dht", "control": "temperature",
                                       "value": round(temp, 1)}})
        steps.append({"set-control": {"part-id": "dht", "control": "humidity",
                                       "value": fixed_air_humidity}})
        steps.append({"set-control": {"part-id": "pot", "control": "position",
                                       "value": round(position, 4)}})
        steps.append({"delay": f"{step_ms}ms"})

    # Phase 1: drift away from comfortable towards both trigger thresholds.
    for i in range(n_up + 1):
        frac = i / n_up
        set_env(start_temp + frac * (peak_temp - start_temp),
                start_soil_pct + frac * (trough_soil_pct - start_soil_pct))

    # Phase 2: hold at the extreme so the ON state is clearly visible,
    # not just a single instantaneous crossing.
    for _ in range(n_hold):
        set_env(peak_temp, trough_soil_pct)

    # Phase 3: drift back to comfortable, to also capture the OFF
    # transition -- a recording that only shows things turning ON tells
    # half the story of the hysteresis dead band.
    for i in range(n_down + 1):
        frac = i / n_down
        set_env(peak_temp - frac * (peak_temp - start_temp),
                trough_soil_pct + frac * (start_soil_pct - trough_soil_pct))

    return steps


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", default="drift_scenario.yaml")
    parser.add_argument("--start-temp", type=float, default=22.0,
                         help="Starting temperature, degC (comfortable)")
    parser.add_argument("--peak-temp", type=float, default=28.0,
                         help="Peak temperature, degC (above fan ON's 26.0 "
                              "default -- see thresholds.h)")
    parser.add_argument("--start-soil", type=float, default=70.0,
                         help="Starting soil moisture, percent (comfortable)")
    parser.add_argument("--trough-soil", type=float, default=15.0,
                         help="Driest soil moisture, percent (below pump "
                              "ON's 30%% default -- see thresholds.h)")
    parser.add_argument("--air-humidity", type=float, default=55.0,
                         help="Fixed DHT22 air humidity -- cosmetic only, "
                              "not used by any threshold in this firmware")
    parser.add_argument("--step-ms", type=int, default=2000,
                         help="Simulated ms between each small change. "
                              "2000 matches the firmware's own sampling "
                              "cadence (Timer1 in main.c), so every step "
                              "shows up as a distinct telemetry reading")
    parser.add_argument("--n-up", type=int, default=20,
                         help="Number of steps drifting AWAY from comfortable")
    parser.add_argument("--n-hold", type=int, default=5,
                         help="Number of steps held at the extreme")
    parser.add_argument("--n-down", type=int, default=20,
                         help="Number of steps drifting back to comfortable")
    args = parser.parse_args()

    steps = build_steps(args.start_temp, args.peak_temp,
                         args.start_soil, args.trough_soil,
                         args.step_ms, args.n_up, args.n_hold, args.n_down,
                         args.air_humidity)

    scenario = {
        "name": "Derive environnementale automatique (serre)",
        "version": 1,
        "author": "Horacia",
        "steps": steps,
    }

    with open(args.output, "w", encoding="utf-8") as f:
        yaml.dump(scenario, f, sort_keys=False, allow_unicode=True)

    n_points = args.n_up + args.n_hold + args.n_down + 2
    total_s = n_points * args.step_ms / 1000
    print(f"Scenario ecrit -> {args.output}")
    print(f"  {n_points} points de derive, {args.step_ms} ms chacun "
          f"-> ~{total_s:.0f} s de temps simule au total")
    print(f"  Temperature : {args.start_temp}C -> {args.peak_temp}C -> "
          f"{args.start_temp}C")
    print(f"  Sol         : {args.start_soil}% -> {args.trough_soil}% -> "
          f"{args.start_soil}%")
    print(f"  --timeout a utiliser avec wokwi-cli : au moins {int(total_s*1000)+5000}")


if __name__ == "__main__":
    main()
