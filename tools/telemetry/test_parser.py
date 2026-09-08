"""Run: python3 test_parser.py -- no pytest, no serial port, no hardware.

Same testing philosophy as the C side of this project: verify the pure
decision (what does this line mean) on its own.
"""
from parser import parse_line

failures = 0


def check(desc: str, cond: bool) -> None:
    global failures
    status = "OK" if cond else "FAIL"
    print(f"  [{status}] {desc}")
    if not cond:
        failures += 1


print("--- parse_line() ---")

check(
    "parses a normal reading",
    parse_line("T:26C | Air:45% | Soil:55% | Fan:ON | Pump:OFF") == {
        "temperature_c": 26, "air_humidity_percent": 45,
        "soil_humidity_percent": 55, "fan_on": True, "pump_on": False,
    },
)

check(
    "parses a negative temperature",
    parse_line("T:-5C | Air:80% | Soil:20% | Fan:OFF | Pump:ON")["temperature_c"] == -5,
)

check(
    "both actuators off parses cleanly",
    parse_line("T:20C | Air:50% | Soil:50% | Fan:OFF | Pump:OFF") is not None,
)

check(
    "both actuators on parses cleanly",
    parse_line("T:30C | Air:50% | Soil:10% | Fan:ON | Pump:ON") is not None,
)

check(
    "a GET command response is not telemetry",
    parse_line("FAN_ON=260 FAN_OFF=240 PUMP_ON=30 PUMP_OFF=60") is None,
)

check("an OK response is not telemetry", parse_line("OK") is None)

check(
    "an error response is not telemetry",
    parse_line("ERR rejected: would make thresholds invalid") is None,
)

check("the boot message is not telemetry", parse_line("System Active") is None)

check("empty line is not telemetry", parse_line("") is None)

check(
    "garbage / line noise does not raise, just returns None",
    parse_line("\x00\x01garbled") is None,
)

check(
    "trailing carriage return / newline is tolerated",
    parse_line("T:26C | Air:45% | Soil:55% | Fan:ON | Pump:OFF\r\n") is not None,
)

print()
if failures == 0:
    print("=== all tests passed ===")
else:
    print(f"=== {failures} test(s) FAILED ===")
    raise SystemExit(1)
