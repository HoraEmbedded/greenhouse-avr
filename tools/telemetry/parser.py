"""Pure parsing logic for one line of firmware telemetry.

No serial port, no file I/O: this is what makes it testable without any
hardware attached (test_parser.py), the same principle used throughout
this project's C code -- separate the DECISION (here, "what does this
line mean") from the I/O that carries it.

Two line shapes are recognised, both landing in the same Reading
schema so a single CSV can hold both:

Normal reading, exactly as main.c emits it on a successful DHT22 read:
    T:<int>C | Air:<int>% | Soil:<int>% | Fan:<ON|OFF> | Pump:<ON|OFF> | Water:<OK|LOW> | Hour:<int>

Sensor-error reading, emitted during a prolonged DHT22 failure (see
fault_handling.c): no temperature or air humidity to report, so those
two fields are None rather than a made-up number, and
sensor_error/consecutive_failures record that this row is a fault
report, not a normal sample:
    SENSOR_ERR consecutive=<int> | Fan:<ON|OFF> | Soil:<int>% | Pump:<ON|OFF> | Water:<OK|LOW> | Hour:<int>

This used to fall through to None like any other non-telemetry line --
a documented gap that meant sensor-outage periods were invisible in the
logged CSV. Recognising it here closes that gap.

Any OTHER line (a command response like "OK", the boot message, serial
noise while the port opens) still returns None rather than raising: a
logger reading a live port sees plenty of lines that are not telemetry,
and that is normal, not an error.
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
    """Parses one telemetry line, normal or sensor-error. Returns None if
    the line matches neither shape -- never raises on malformed input."""
    stripped = line.strip()

    match = _NORMAL_PATTERN.match(stripped)
    if match is not None:
        temp, air, soil, fan, pump, water, hour = match.groups()
        return Reading(
            temperature_c=int(temp),
            air_humidity_percent=int(air),
            soil_humidity_percent=int(soil),
            fan_on=(fan == "ON"),
            pump_on=(pump == "ON"),
            water_ok=(water == "OK"),
            hour=int(hour),
            sensor_error=False,
            consecutive_failures=0,
        )

    match = _SENSOR_ERROR_PATTERN.match(stripped)
    if match is not None:
        consecutive, fan, soil, pump, water, hour = match.groups()
        return Reading(
            temperature_c=None,
            air_humidity_percent=None,
            soil_humidity_percent=int(soil),
            fan_on=(fan == "ON"),
            pump_on=(pump == "ON"),
            water_ok=(water == "OK"),
            hour=int(hour),
            sensor_error=True,
            consecutive_failures=int(consecutive),
        )

    return None
