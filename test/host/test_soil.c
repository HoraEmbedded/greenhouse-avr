/* Host-side unit tests for soil_percent_from_raw() with the DEFAULT
 * calibration from soil.h (0 -> 0 %, 1023 -> 100 %), which is what the
 * Wokwi potentiometer actually behaves like.
 *
 * A realistic PARTIAL, possibly INVERTED calibration (what a real
 * capacitive probe will need) is tested separately in
 * test_soil_calibrated.c, compiled with overridden -D values -- see
 * the Makefile. Clamping past either endpoint isn't reachable at all
 * under this default calibration (it spans the ADC's full 0-1023 range,
 * so there is no raw value beyond it), which is exactly why that case
 * belongs in the other file, not this one.
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
    printf("--- soil_percent_from_raw(), default calibration "
           "(%d -> 0%%, %d -> 100%%) ---\n",
           SOIL_ADC_AT_0_PERCENT, SOIL_ADC_AT_100_PERCENT);

    CHECK("raw 0 reads exactly 0%", soil_percent_from_raw(0) == 0);
    CHECK("raw 1023 reads exactly 100%", soil_percent_from_raw(1023) == 100);

    int8_t mid_pct = soil_percent_from_raw(512);
    CHECK("the midpoint (raw 512) reads close to 50%",
          mid_pct >= 49 && mid_pct <= 51);

    int8_t low_pct = soil_percent_from_raw(100);
    CHECK("a low raw value reads a low percentage",
          low_pct >= 8 && low_pct <= 11);

    int8_t high_pct = soil_percent_from_raw(900);
    CHECK("a high raw value reads a high percentage",
          high_pct >= 87 && high_pct <= 89);

    printf("\n");
    if (failures == 0) {
        printf("=== all tests passed ===\n");
        return 0;
    }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
