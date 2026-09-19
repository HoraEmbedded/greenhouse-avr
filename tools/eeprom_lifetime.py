"""EEPROM wear budget for threshold storage.

eeprom_config_save() uses eeprom_update_*, which only writes a byte if
its value changed. The checksum byte wears fastest: XOR algebra
guarantees it changes whenever any of the 6 threshold bytes changes
(b XOR b' != 0 whenever b != b'), so it's written on every SET/RESET
that actually changes something -- more often than any single field.

Run: python eeprom_lifetime.py --commands-per-day 20
"""
import argparse

EEPROM_ENDURANCE_CYCLES = 100_000
DAYS_PER_YEAR = 365.25


def years_until_wear_limit(commands_per_day: float) -> float:
    if commands_per_day <= 0:
        return float("inf")
    return (EEPROM_ENDURANCE_CYCLES / commands_per_day) / DAYS_PER_YEAR


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--commands-per-day", type=float, default=20)
    args = parser.parse_args()

    years = years_until_wear_limit(args.commands_per_day)

    print("Fastest-wearing byte: checksum (changes on every real SET/RESET, "
          "proven by XOR algebra).")
    print(f"EEPROM endurance: {EEPROM_ENDURANCE_CYCLES:,} write cycles")
    print(f"\nAssumption: {args.commands_per_day:.0f} command(s)/day")
    print(f"=> Years until limit: {years:,.0f}")

    print("\nScenarios:")
    for name, cmds in [("normal use", 5), ("heavy use", 50), ("abuse", 500)]:
        print(f"  {name:<10} ({cmds:>3}/day) -> {years_until_wear_limit(cmds):>8,.0f} years")


if __name__ == "__main__":
    main()
