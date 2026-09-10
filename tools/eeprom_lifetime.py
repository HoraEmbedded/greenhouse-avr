"""EEPROM wear budget for the greenhouse firmware's threshold storage.

eeprom_config_save() uses eeprom_update_byte()/eeprom_update_block()
throughout (see src/eeprom_config.c), which only performs a real EEPROM
write when a byte's value actually changes -- so the EEPROM's bytes do
NOT all wear at the same rate:

  - The magic byte never changes after the very first successful save
    (it is always the same constant, 0xA5), so avr-libc's update-not-
    write behaviour means it gets a real write exactly ONCE, ever, for
    the life of the device.
  - The 6 threshold bytes only take a real write when THEIR OWN field
    changes: "SET FAN_ON ..." only touches fan_on_decidegC's 1-2 bytes,
    not the other 4.
  - The checksum byte is the one that wears fastest, and this is
    provable, not just likely: it is an XOR over all 6 threshold bytes,
    and for any single byte b that changes to b' (b != b'), b XOR b' is
    never zero -- so the running XOR necessarily differs whenever ANY
    of the 6 bytes changes. It therefore takes a real write on every
    SET/RESET that changes the configuration at all, which is at least
    as often as any single threshold byte, and usually more often.

This script computes how many years of realistic use it would take to
reach the ATmega2560 datasheet's 100 000 write/erase cycle EEPROM
endurance limit on that one fastest-wearing byte -- the number that
actually bounds the device's configuration-write lifetime, not a
generic "EEPROM lasts 100 000 writes" statement that doesn't say which
100 000.

Run: python eeprom_lifetime.py --commands-per-day 20
"""
import argparse

EEPROM_ENDURANCE_CYCLES = 100_000  # ATmega2560 datasheet: EEPROM write/erase cycles
DAYS_PER_YEAR = 365.25


def years_until_wear_limit(commands_per_day: float) -> float:
    """commands_per_day: SET/RESET commands per day that actually change
    at least one threshold (a command that doesn't change anything -- a
    RESET when already at defaults, a SET to the current value -- costs
    nothing here, since eeprom_update_* skips unchanged bytes)."""
    if commands_per_day <= 0:
        return float("inf")
    days = EEPROM_ENDURANCE_CYCLES / commands_per_day
    return days / DAYS_PER_YEAR


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--commands-per-day", type=float, default=20,
                         help="Realistic upper bound on SET/RESET commands "
                              "per day that actually change a threshold")
    args = parser.parse_args()

    years = years_until_wear_limit(args.commands_per_day)

    print("Octet le plus sollicité : le checksum -- change a chaque "
          "SET/RESET qui modifie reellement un seuil, garanti par "
          "l'algebre XOR (b XOR b' != 0 des que b != b'), pas suppose.")
    print(f"Endurance EEPROM (datasheet ATmega2560) : "
          f"{EEPROM_ENDURANCE_CYCLES:,} cycles d'ecriture/effacement")
    print(f"\nHypothese : {args.commands_per_day:.0f} commande(s)/jour "
          f"changeant reellement un seuil")
    print(f"=> Duree avant la limite : {years:,.0f} ans")

    print("\nQuelques scenarios pour comparaison :")
    for scenario_name, cmds in [("usage normal", 5), ("usage intensif", 50),
                                  ("abus delibere", 500)]:
        y = years_until_wear_limit(cmds)
        print(f"  {scenario_name:<16} ({cmds:>3} cmd/jour) -> {y:>10,.0f} ans")


if __name__ == "__main__":
    main()
