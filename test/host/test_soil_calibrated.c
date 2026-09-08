/* This file is compiled TWICE by the Makefile, with different -D
 * overrides for SOIL_ADC_AT_0_PERCENT / SOIL_ADC_AT_100_PERCENT: once as
 * a realistic PARTIAL calibration (probe doesn't use the full ADC
 * range), once INVERTED (a real capacitive probe often reads a HIGHER
 * raw value when the soil is DRIER -- the opposite of the Wokwi
 * potentiometer). Same source, same assertions, two calibrations: if the
 * formula in soil.c only worked for one direction, one of the two builds
 * would fail.
 */

#include <stdio.h>
#include <stdint.h>
#include "../../src/soil.h"

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
    printf("--- soil_percent_from_raw(), calibration under test: "
           "%d -> 0%%, %d -> 100%% ---\n",
           SOIL_ADC_AT_0_PERCENT, SOIL_ADC_AT_100_PERCENT);

    CHECK("the 0% calibration point reads exactly 0",
          soil_percent_from_raw(SOIL_ADC_AT_0_PERCENT) == 0);

    CHECK("the 100% calibration point reads exactly 100",
          soil_percent_from_raw(SOIL_ADC_AT_100_PERCENT) == 100);

    /* Step away from each end, in whichever direction is "further from
     * 0%"/"further from 100%" for THIS calibration's polarity. */
    int32_t span = (int32_t)SOIL_ADC_AT_100_PERCENT - SOIL_ADC_AT_0_PERCENT;
    int16_t step = (span > 0) ? 30 : -30;

    uint16_t beyond_100 = (uint16_t)((int32_t)SOIL_ADC_AT_100_PERCENT + step);
    uint16_t beyond_0   = (uint16_t)((int32_t)SOIL_ADC_AT_0_PERCENT - step);

    CHECK("a raw value beyond the 100% point clamps to 100, not more",
          soil_percent_from_raw(beyond_100) == 100);

    CHECK("a raw value beyond the 0% point clamps to 0, not less",
          soil_percent_from_raw(beyond_0) == 0);

    printf("\n");
    if (failures == 0) {
        printf("=== all tests passed ===\n");
        return 0;
    }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
