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
        b"T:22C | Air:60% | Soil:70% | Fan:OFF | Pump:OFF\r\n",
        b"OK\r\n",                                                        # a command response, ignored
        b"T:27C | Air:58% | Soil:68% | Fan:ON | Pump:OFF\r\n",
        b"T:20C | Air:59% | Soil:28% | Fan:OFF | Pump:ON\r\n",
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

    print("--- logger.run() against a fake serial port ---")
    check("exactly 3 telemetry rows written (boot message and OK skipped)",
          len(rows) == 3)
    check("first row has the right temperature",
          rows[0]["temperature_c"] == "22")
    check("second row shows the fan on",
          rows[1]["fan_on"] == "True")
    check("third row shows the pump on",
          rows[2]["pump_on"] == "True")
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
