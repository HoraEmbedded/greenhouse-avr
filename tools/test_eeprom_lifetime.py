"""Run: python test_eeprom_lifetime.py"""
from eeprom_lifetime import years_until_wear_limit, EEPROM_ENDURANCE_CYCLES

failures = 0


def check(desc, cond):
    global failures
    print(f"  [{'OK' if cond else 'FAIL'}] {desc}")
    if not cond:
        failures += 1


print("--- years_until_wear_limit() ---")
check("0/day never reaches limit", years_until_wear_limit(0) == float("inf"))
check("1/day matches endurance/365.25",
      abs(years_until_wear_limit(1) - EEPROM_ENDURANCE_CYCLES / 365.25) < 0.01)
check("double rate halves years",
      abs(years_until_wear_limit(20) - years_until_wear_limit(10) / 2) < 0.01)

print()
if failures == 0:
    print("=== all tests passed ===")
else:
    print(f"=== {failures} test(s) FAILED ===")
    raise SystemExit(1)
