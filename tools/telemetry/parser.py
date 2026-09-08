"""Pure parsing logic for one line of firmware telemetry.

No serial port, no file I/O: this is what makes it testable without any
hardware attached (test_parser.py), the same principle used throughout
this project's C code -- separate the DECISION (here, "what does this
line mean") from the I/O that carries it.

Expected line, exactly as main.c emits it:
    T:<int>C | Air:<int>% | Soil:<int>% | Fan:<ON|OFF> | Pump:<ON|OFF>

Any other line (a command response like "OK", the boot message, serial
noise while the port opens) returns None rather than raising: a logger
reading a live port sees plenty of lines that are not telemetry, and
that is normal, not an error.
"""
import re
from typing import Optional, TypedDict


class Reading(TypedDict):
    temperature_c: int
    air_humidity_percent: int
    soil_humidity_percent: int
    fan_on: bool
    pump_on: bool


_PATTERN = re.compile(
    r"^T:(-?\d+)C \| Air:(-?\d+)% \| Soil:(-?\d+)% \| "
    r"Fan:(ON|OFF) \| Pump:(ON|OFF)$"
)


def parse_line(line: str) -> Optional[Reading]:
    """Parses one telemetry line. Returns None if the line does not match
    the expected format -- never raises on malformed input."""
    match = _PATTERN.match(line.strip())
    if match is None:
        return None

    temp, air, soil, fan, pump = match.groups()
    return Reading(
        temperature_c=int(temp),
        air_humidity_percent=int(air),
        soil_humidity_percent=int(soil),
        fan_on=(fan == "ON"),
        pump_on=(pump == "ON"),
    )
