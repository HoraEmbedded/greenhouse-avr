"""Integration test for logger.run() against a fake serial port.
Run: python test_logger_integration.py
"""
import csv
import io

from logger import run


class FakeSerial:
    def __init__(self, lines):
        self._lines = list(lines)

    def readline(self):
        if not self._lines:
            raise StopIteration
        return self._lines.pop(0)


def main() -> None:
    lines = [
        b"System Active\r\n",
        b"T:22C | Air:60% | Soil:70% | Fan:OFF | Pump:OFF | Water:OK | Hour:9\r\n",
        b"OK\r\n",
        b"T:27C | Air:58% | Soil:68% | Fan:ON | Pump:OFF | Water:OK | Hour:13\r\n",
        b"T:20C | Air:59% | Soil:28% | Fan:OFF | Pump:ON | Water:LOW | Hour:22\r\n",
        b"SENSOR_ERR consecutive=3 | Fan:ON | Soil:25% | Pump:OFF | Water:OK | Hour:23\r\n",
    ]

    fake_port = FakeSerial(lines)
    csv_buffer = io.StringIO()

    try:
        run(fake_port, csv_buffer, verbose=False)
    except StopIteration:
        pass

    csv_buffer.seek(0)
    rows = list(csv.DictReader(csv_buffer))
    failures = 0

    def check(desc, cond):
        nonlocal failures
        print(f"  [{'OK' if cond else 'FAIL'}] {desc}")
        if not cond:
            failures += 1

    print("--- logger.run() against a fake serial port ---\n")
    check("4 telemetry rows (boot/OK skipped)", len(rows) == 4)
    check("first row temperature", rows[0]["temperature_c"] == "22")
    check("second row fan on", rows[1]["fan_on"] == "True")
    check("third row pump on", rows[2]["pump_on"] == "True")
    check("third row water low", rows[2]["water_ok"] == "False")
    check("hour varies per row", rows[0]["hour"] == "9" and rows[2]["hour"] == "22")
    check("SENSOR_ERR row captured", rows[3]["sensor_error"] == "True" and rows[3]["consecutive_failures"] == "3")
    check("SENSOR_ERR has no temperature", rows[3]["temperature_c"] == "")
    check("normal rows sensor_error=False", rows[0]["sensor_error"] == "False")
    check("every row has a timestamp", all(row["timestamp_utc"] for row in rows))

    print()
    if failures == 0:
        print("=== all tests passed ===")
    else:
        print(f"=== {failures} test(s) FAILED ===")
        raise SystemExit(1)


if __name__ == "__main__":
    main()
