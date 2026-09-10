"""Integration test for logger.run(): drives it with a fake serial port
(no hardware, no pyserial connection) and checks the resulting CSV.

This is the one test in this project that exercises the full path --
read a line, parse it, write a row -- but it does so without ever opening
a real port, which is what makes it runnable in CI.
"""
import csv
import io

from logger import run


class FakeSerial:
    """Stands in for a pyserial Serial object: readline() returns queued
    lines one at a time, then raises to end the loop -- a real port would
    just block forever, which a test can't do."""

    def __init__(self, lines):
        self._lines = list(lines)

    def readline(self):
        if not self._lines:
            raise StopIteration
        return self._lines.pop(0)


def main() -> None:
    lines = [
        b"System Active\r\n",                                            # boot message, ignored
        b"T:22C | Air:60% | Soil:70% | Fan:OFF | Pump:OFF | Water:OK | Hour:9\r\n",
        b"OK\r\n",                                                        # a command response, ignored
        b"T:27C | Air:58% | Soil:68% | Fan:ON | Pump:OFF | Water:OK | Hour:13\r\n",
        b"T:20C | Air:59% | Soil:28% | Fan:OFF | Pump:ON | Water:LOW | Hour:22\r\n",
        b"SENSOR_ERR consecutive=3 | Fan:ON | Soil:25% | Pump:OFF | Water:OK | Hour:23\r\n",
    ]

    fake_port = FakeSerial(lines)
    csv_buffer = io.StringIO()

    try:
        run(fake_port, csv_buffer, verbose=False)
    except StopIteration:
        pass  # expected: FakeSerial signals end of input this way

    csv_buffer.seek(0)
    rows = list(csv.DictReader(csv_buffer))

    failures = 0

    def check(desc, cond):
        nonlocal failures
        print(f"  [{'OK' if cond else 'FAIL'}] {desc}")
        if not cond:
            failures += 1

    print("--- logger.run() against a fake serial port ---\n")
    check("exactly 4 telemetry rows written (boot message and OK skipped)",
          len(rows) == 4)
    check("first row has the right temperature",
          rows[0]["temperature_c"] == "22")
    check("second row shows the fan on",
          rows[1]["fan_on"] == "True")
    check("third row shows the pump on",
          rows[2]["pump_on"] == "True")
    check("third row's low water level is captured, not dropped",
          rows[2]["water_ok"] == "False")
    check("hour is captured correctly across rows, not a fixed default",
          rows[0]["hour"] == "9" and rows[2]["hour"] == "22")
    check("the fourth row (SENSOR_ERR) is captured, not silently dropped "
          "-- this used to be a documented gap",
          rows[3]["sensor_error"] == "True" and
          rows[3]["consecutive_failures"] == "3")
    check("the SENSOR_ERR row has no temperature (empty, not a made-up "
          "value written into the CSV)",
          rows[3]["temperature_c"] == "")
    check("normal rows are marked sensor_error=False, not left blank",
          rows[0]["sensor_error"] == "False")
    check("every row has a timestamp",
          all(row["timestamp_utc"] for row in rows))

    print()
    if failures == 0:
        print("=== all tests passed ===")
    else:
        print(f"=== {failures} test(s) FAILED ===")
        raise SystemExit(1)


if __name__ == "__main__":
    main()
