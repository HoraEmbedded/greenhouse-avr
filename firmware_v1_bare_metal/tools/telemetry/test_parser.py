"""Run: python test_parser.py"""
from parser import parse_line

failures = 0


def check(desc, cond):
    global failures
    print(f"  [{'OK' if cond else 'FAIL'}] {desc}")
    if not cond:
        failures += 1


print("--- parse_line() ---")

check("parses a normal reading",
      parse_line("T:26C | Air:45% | Soil:55% | Fan:ON | Pump:OFF | Water:OK | Hour:14") == {
          "temperature_c": 26, "air_humidity_percent": 45, "soil_humidity_percent": 55,
          "fan_on": True, "pump_on": False, "water_ok": True, "hour": 14,
          "sensor_error": False, "consecutive_failures": 0,
      })
check("negative temperature", parse_line("T:-5C | Air:80% | Soil:20% | Fan:OFF | Pump:ON | Water:OK | Hour:14")["temperature_c"] == -5)
check("both off parses", parse_line("T:20C | Air:50% | Soil:50% | Fan:OFF | Pump:OFF | Water:OK | Hour:14") is not None)
check("both on parses", parse_line("T:30C | Air:50% | Soil:10% | Fan:ON | Pump:ON | Water:OK | Hour:14") is not None)
check("Water:LOW decodes", parse_line("T:25C | Air:50% | Soil:15% | Fan:OFF | Pump:OFF | Water:LOW")  is None)  # missing Hour
check("hour field decoded", parse_line("T:25C | Air:50% | Soil:15% | Fan:OFF | Pump:OFF | Water:OK | Hour:3")["hour"] == 3)
check("GET reply is not telemetry", parse_line("FAN_ON=260 FAN_OFF=240 PUMP_ON=30 PUMP_OFF=60") is None)
check("OK reply is not telemetry", parse_line("OK") is None)
check("ERR reply is not telemetry", parse_line("ERR rejected") is None)
check("boot message is not telemetry", parse_line("System Active") is None)
check("empty line is not telemetry", parse_line("") is None)
check("garbage doesn't raise", parse_line("\x00\x01garbled") is None)
check("trailing CRLF tolerated", parse_line("T:26C | Air:45% | Soil:55% | Fan:ON | Pump:OFF | Water:OK | Hour:14\r\n") is not None)
check("old format (no Hour) no longer parses", parse_line("T:26C | Air:45% | Soil:55% | Fan:ON | Pump:OFF") is None)

check("SENSOR_ERR recognised",
      parse_line("SENSOR_ERR consecutive=3 | Fan:ON | Soil:20% | Pump:OFF | Water:OK | Hour:14") == {
          "temperature_c": None, "air_humidity_percent": None, "soil_humidity_percent": 20,
          "fan_on": True, "pump_on": False, "water_ok": True, "hour": 14,
          "sensor_error": True, "consecutive_failures": 3,
      })
check("normal reading has sensor_error=False", parse_line("T:26C | Air:45% | Soil:55% | Fan:ON | Pump:OFF | Water:OK | Hour:14")["sensor_error"] is False)
check("malformed SENSOR_ERR rejected", parse_line("SENSOR_ERR consecutive=3 | Fan:ON | Soil:20% | Pump:OFF") is None)

print()
if failures == 0:
    print("=== all tests passed ===")
else:
    print(f"=== {failures} test(s) FAILED ===")
    raise SystemExit(1)
