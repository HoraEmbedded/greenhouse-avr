/* Host-side unit tests for degraded_fan_state() in fault_handling.c. */

#include <stdio.h>
#include "../../src/fault_handling.h"

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
    printf("--- degraded_fan_state() ---\n");

    CHECK("a single failure (below threshold) keeps the fan OFF as it was",
          degraded_fan_state(1, 0) == 0);

    CHECK("a single failure (below threshold) keeps the fan ON as it was",
          degraded_fan_state(1, 1) == 1);

    CHECK("just below the threshold still keeps the last known state",
          degraded_fan_state(DHT_FAILURE_SAFETY_THRESHOLD - 1, 0) == 0);

    CHECK("exactly at the threshold forces the fan ON, "
          "even though it was OFF",
          degraded_fan_state(DHT_FAILURE_SAFETY_THRESHOLD, 0) == 1);

    CHECK("well past the threshold, still forced ON",
          degraded_fan_state(DHT_FAILURE_SAFETY_THRESHOLD + 50, 0) == 1);

    CHECK("at the threshold, a fan already ON stays ON (no-op, not a bug)",
          degraded_fan_state(DHT_FAILURE_SAFETY_THRESHOLD, 1) == 1);

    printf("\n");
    if (failures == 0) {
        printf("=== all tests passed ===\n");
        return 0;
    }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
