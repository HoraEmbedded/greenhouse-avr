"""Pure parser for one telemetry line. No I/O -- testable on the host.

Formats:
  T:<int>C | Air:<int>% | Soil:<int>% | Fan:ON|OFF | Pump:ON|OFF | Water:OK|LOW | Hour:<int>
  SENSOR_ERR consecutive=<int> | Fan:ON|OFF | Soil:<int>% | Pump:ON|OFF | Water:OK|LOW | Hour:<int>

Anything else (command replies, boot message, noise) returns None.
"""
import re
from typing import Optional, TypedDict


class Reading(TypedDict):
    temperature_c: Optional[int]
    air_humidity_percent: Optional[int]
    soil_humidity_percent: int
    fan_on: bool
    pump_on: bool
    water_ok: bool
    hour: int
    sensor_error: bool
    consecutive_failures: int


_NORMAL_PATTERN = re.compile(
    r"^T:(-?\d+)C \| Air:(-?\d+)% \| Soil:(-?\d+)% \| "
    r"Fan:(ON|OFF) \| Pump:(ON|OFF) \| Water:(OK|LOW) \| Hour:(\d+)$"
)

_SENSOR_ERROR_PATTERN = re.compile(
    r"^SENSOR_ERR consecutive=(\d+) \| Fan:(ON|OFF) \| Soil:(-?\d+)% \| "
    r"Pump:(ON|OFF) \| Water:(OK|LOW) \| Hour:(\d+)$"
)


def parse_line(line: str) -> Optional[Reading]:
    stripped = line.strip()

    match = _NORMAL_PATTERN.match(stripped)
    if match is not None:
        temp, air, soil, fan, pump, water, hour = match.groups()
        return Reading(
            temperature_c=int(temp), air_humidity_percent=int(air),
            soil_humidity_percent=int(soil), fan_on=(fan == "ON"),
            pump_on=(pump == "ON"), water_ok=(water == "OK"), hour=int(hour),
            sensor_error=False, consecutive_failures=0,
        )

    match = _SENSOR_ERROR_PATTERN.match(stripped)
    if match is not None:
        consecutive, fan, soil, pump, water, hour = match.groups()
        return Reading(
            temperature_c=None, air_humidity_percent=None,
            soil_humidity_percent=int(soil), fan_on=(fan == "ON"),
            pump_on=(pump == "ON"), water_ok=(water == "OK"), hour=int(hour),
            sensor_error=True, consecutive_failures=int(consecutive),
        )

    return None
