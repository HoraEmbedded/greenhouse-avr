#include <stdio.h>
#include <stdint.h>
#include "../../src/soil.h"

static int failures = 0;
#define CHECK(desc, cond) do { if (cond) printf("  [OK] %s\n", desc); else { printf("  [FAIL] %s\n", desc); failures++; } } while (0)

int main(void) {
    printf("--- soil_percent_from_raw(), default calibration ---\n");
    CHECK("raw 0 -> 0%%", soil_percent_from_raw(0) == 0);
    CHECK("raw 1023 -> 100%%", soil_percent_from_raw(1023) == 100);
    int8_t mid = soil_percent_from_raw(512);
    CHECK("midpoint ~50%%", mid >= 49 && mid <= 51);
    int8_t low = soil_percent_from_raw(100);
    CHECK("low raw -> low %%", low >= 8 && low <= 11);
    int8_t high = soil_percent_from_raw(900);
    CHECK("high raw -> high %%", high >= 87 && high <= 89);
    printf("\n");
    if (failures == 0) { printf("=== all tests passed ===\n"); return 0; }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
