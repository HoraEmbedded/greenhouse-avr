/* Host-side unit tests for thresholds_valid() in thresholds.c. */

#include <stdio.h>
#include <string.h>
#include "../../src/thresholds.h"

static int failures = 0;

#define CHECK(desc, cond)                                              \
    do {                                                               \
        if (cond) {                                                    \
            printf("  [OK] %s\n", desc);                               \
        } else {                                                       \
            printf("  [FAIL] %s\n", desc);                             \
            failures++;                                                \
        }                                                               \
    } while (0)

int main(void) {
    printf("--- thresholds_valid() ---\n");

    GreenhouseThresholds defaults = GREENHOUSE_DEFAULT_THRESHOLDS;
    CHECK("factory defaults are valid", thresholds_valid(&defaults) == 1);

    /* Struct layout matters because this struct is written byte-for-byte
     * into EEPROM (see eeprom_config.c): confirm AVR-GCC's assumed layout
     * (no padding) really is what sizeof() reports, on this compiler too. */
    CHECK("struct has no hidden padding (int16+int16+int8+int8 = 6 bytes)",
          sizeof(GreenhouseThresholds) == 6);

    GreenhouseThresholds fan_inverted = defaults;
    fan_inverted.fan_on_decidegC = 240;
    fan_inverted.fan_off_decidegC = 260;
    CHECK("fan ON <= OFF is rejected (would invert the dead band)",
          thresholds_valid(&fan_inverted) == 0);

    GreenhouseThresholds fan_equal = defaults;
    fan_equal.fan_on_decidegC = 250;
    fan_equal.fan_off_decidegC = 250;
    CHECK("fan ON == OFF is rejected (would collapse the dead band)",
          thresholds_valid(&fan_equal) == 0);

    GreenhouseThresholds pump_inverted = defaults;
    pump_inverted.pump_on_percent = 60;
    pump_inverted.pump_off_percent = 30;
    CHECK("pump ON >= OFF is rejected",
          thresholds_valid(&pump_inverted) == 0);

    GreenhouseThresholds pump_out_of_range = defaults;
    pump_out_of_range.pump_off_percent = 120;
    CHECK("pump percent above 100 is rejected",
          thresholds_valid(&pump_out_of_range) == 0);

    GreenhouseThresholds pump_negative = defaults;
    pump_negative.pump_on_percent = -5;
    CHECK("negative pump percent is rejected",
          thresholds_valid(&pump_negative) == 0);

    GreenhouseThresholds temp_too_hot = defaults;
    temp_too_hot.fan_on_decidegC = 900; /* above the DHT22 ceiling */
    CHECK("temperature above sensor range is rejected",
          thresholds_valid(&temp_too_hot) == 0);

    GreenhouseThresholds temp_too_cold = defaults;
    temp_too_cold.fan_off_decidegC = -500; /* below the DHT22 floor */
    CHECK("temperature below sensor range is rejected",
          thresholds_valid(&temp_too_cold) == 0);

    /* The other side of each range check where it's actually reachable
     * (see the coverage note above thresholds_valid() for the two
     * branches where it is NOT: fan_off > MAX and pump_off < 0). Each
     * case below pairs the two fields so the ordering check (line 4 or
     * 10) passes WITHOUT already deciding the outcome -- otherwise the
     * test would pass for the wrong reason, catching the input one
     * branch earlier than intended. */
    GreenhouseThresholds fan_on_too_cold = defaults;
    fan_on_too_cold.fan_on_decidegC = -410;
    fan_on_too_cold.fan_off_decidegC = -420; /* still below fan_on: line 4 passes */
    CHECK("fan ON below sensor range is rejected (and by the intended "
          "branch: fan_off is set below it, so the ordering check alone "
          "cannot explain the rejection)",
          thresholds_valid(&fan_on_too_cold) == 0);

    GreenhouseThresholds pump_on_too_high = defaults;
    pump_on_too_high.pump_on_percent = 150;
    pump_on_too_high.pump_off_percent = 200; /* still above pump_on: line 10 passes */
    CHECK("pump ON above 100 is rejected (by the intended branch, "
          "same reasoning)",
          thresholds_valid(&pump_on_too_high) == 0);

    printf("\n");
    if (failures == 0) {
        printf("=== all tests passed ===\n");
        return 0;
    }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
