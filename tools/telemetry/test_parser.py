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
    parse_line("T:26C | Air:45% | Soil:55% | Fan:ON | Pump:OFF | Water:OK | Hour:14") == {
        "temperature_c": 26, "air_humidity_percent": 45,
        "soil_humidity_percent": 55, "fan_on": True, "pump_on": False,
        "water_ok": True, "hour": 14,
        "sensor_error": False, "consecutive_failures": 0,
    },
)

check(
    "parses a negative temperature",
    parse_line("T:-5C | Air:80% | Soil:20% | Fan:OFF | Pump:ON | Water:OK | Hour:14")["temperature_c"] == -5,
)

check(
    "both actuators off parses cleanly",
    parse_line("T:20C | Air:50% | Soil:50% | Fan:OFF | Pump:OFF | Water:OK | Hour:14") is not None,
)

check(
    "both actuators on parses cleanly",
    parse_line("T:30C | Air:50% | Soil:10% | Fan:ON | Pump:ON | Water:OK | Hour:14") is not None,
)

check(
    "a low water level is decoded as water_ok=False, not just ignored",
    parse_line("T:25C | Air:50% | Soil:15% | Fan:OFF | Pump:OFF | Water:LOW | Hour:14")["water_ok"] is False,
)

check(
    "the hour field is actually decoded, not hardcoded or ignored",
    parse_line("T:25C | Air:50% | Soil:15% | Fan:OFF | Pump:OFF | Water:OK | Hour:3")["hour"] == 3,
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
    parse_line("T:26C | Air:45% | Soil:55% | Fan:ON | Pump:OFF | Water:OK | Hour:14\r\n") is not None,
)

check(
    "a line in the OLD format (no Water field) no longer parses -- the "
    "format changed, this is not a regression to hide",
    parse_line("T:26C | Air:45% | Soil:55% | Fan:ON | Pump:OFF") is None,
)

check(
    "a SENSOR_ERR line is now recognised as telemetry -- this used to be "
    "a documented gap (see the old comment in parser.py's history), "
    "closed here rather than left silently unrecognised",
    parse_line("SENSOR_ERR consecutive=3 | Fan:ON | Soil:20% | Pump:OFF | Water:OK | Hour:14") is not None,
)

check(
    "a SENSOR_ERR reading has no temperature/air data (None, not a "
    "made-up number) but does have everything else",
    parse_line("SENSOR_ERR consecutive=3 | Fan:ON | Soil:20% | Pump:OFF | Water:OK | Hour:14") == {
        "temperature_c": None, "air_humidity_percent": None,
        "soil_humidity_percent": 20, "fan_on": True, "pump_on": False,
        "water_ok": True, "hour": 14,
        "sensor_error": True, "consecutive_failures": 3,
    },
)

check(
    "a normal reading is marked sensor_error=False with "
    "consecutive_failures=0, not left unset",
    parse_line("T:26C | Air:45% | Soil:55% | Fan:ON | Pump:OFF | Water:OK | Hour:14") == {
        "temperature_c": 26, "air_humidity_percent": 45,
        "soil_humidity_percent": 55, "fan_on": True, "pump_on": False,
        "water_ok": True, "hour": 14,
        "sensor_error": False, "consecutive_failures": 0,
    },
)

check(
    "a malformed SENSOR_ERR line (missing a field) is not recognised, "
    "not partially parsed",
    parse_line("SENSOR_ERR consecutive=3 | Fan:ON | Soil:20% | Pump:OFF") is None,
)

print()
if failures == 0:
    print("=== all tests passed ===")
else:
    print(f"=== {failures} test(s) FAILED ===")
    raise SystemExit(1)
