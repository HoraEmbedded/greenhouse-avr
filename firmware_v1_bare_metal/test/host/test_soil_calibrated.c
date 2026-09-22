/* Compiled twice with different -D overrides (see Makefile): normal
 * and inverted polarity. Same assertions, both must pass. */
#include <stdio.h>
#include <stdint.h>
#include "../../src/soil.h"

static int failures = 0;
#define CHECK(desc, cond) do { if (cond) printf("  [OK] %s\n", desc); else { printf("  [FAIL] %s\n", desc); failures++; } } while (0)

int main(void) {
    printf("--- soil_percent_from_raw(), calibration %d->0%%, %d->100%% ---\n",
           SOIL_ADC_AT_0_PERCENT, SOIL_ADC_AT_100_PERCENT);

    CHECK("0%% point exact", soil_percent_from_raw(SOIL_ADC_AT_0_PERCENT) == 0);
    CHECK("100%% point exact", soil_percent_from_raw(SOIL_ADC_AT_100_PERCENT) == 100);

    int32_t span = (int32_t)SOIL_ADC_AT_100_PERCENT - SOIL_ADC_AT_0_PERCENT;
    int16_t step = (span > 0) ? 30 : -30;
    uint16_t beyond_100 = (uint16_t)((int32_t)SOIL_ADC_AT_100_PERCENT + step);
    uint16_t beyond_0   = (uint16_t)((int32_t)SOIL_ADC_AT_0_PERCENT - step);

    CHECK("clamps at 100%%", soil_percent_from_raw(beyond_100) == 100);
    CHECK("clamps at 0%%", soil_percent_from_raw(beyond_0) == 0);

    printf("\n");
    if (failures == 0) { printf("=== all tests passed ===\n"); return 0; }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
