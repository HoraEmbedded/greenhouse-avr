#include <stdio.h>
#include "../../src/fault_handling.h"

static int failures = 0;
#define CHECK(desc, cond) do { if (cond) printf("  [OK] %s\n", desc); else { printf("  [FAIL] %s\n", desc); failures++; } } while (0)

int main(void) {
    printf("--- degraded_fan_state() ---\n");
    CHECK("below threshold, was off", degraded_fan_state(1, 0) == 0);
    CHECK("below threshold, was on", degraded_fan_state(1, 1) == 1);
    CHECK("just below threshold", degraded_fan_state(DHT_FAILURE_SAFETY_THRESHOLD - 1, 0) == 0);
    CHECK("at threshold forces ON", degraded_fan_state(DHT_FAILURE_SAFETY_THRESHOLD, 0) == 1);
    CHECK("past threshold forces ON", degraded_fan_state(DHT_FAILURE_SAFETY_THRESHOLD + 50, 0) == 1);
    CHECK("at threshold, already on", degraded_fan_state(DHT_FAILURE_SAFETY_THRESHOLD, 1) == 1);
    printf("\n");
    if (failures == 0) { printf("=== all tests passed ===\n"); return 0; }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
