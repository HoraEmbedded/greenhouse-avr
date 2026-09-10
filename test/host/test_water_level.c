/* Host-side unit tests for pump_output_state() in water_level.c. */

#include <stdio.h>
#include "../../src/water_level.h"

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
    printf("--- pump_output_state() ---\n");

    CHECK("water present, hysteresis wants the pump ON: pump runs",
          pump_output_state(1, 1) == 1);

    CHECK("water present, hysteresis wants the pump OFF: pump stays off",
          pump_output_state(0, 1) == 0);

    CHECK("water ABSENT, hysteresis wants the pump ON: forced off anyway "
          "-- this is the whole point of this module",
          pump_output_state(1, 0) == 0);

    CHECK("water absent, hysteresis already OFF: stays off (no surprise "
          "either way)",
          pump_output_state(0, 0) == 0);

    printf("\n");
    if (failures == 0) {
        printf("=== all tests passed ===\n");
        return 0;
    }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
