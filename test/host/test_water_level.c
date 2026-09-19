#include <stdio.h>
#include "../../src/water_level.h"

static int failures = 0;
#define CHECK(desc, cond) do { if (cond) printf("  [OK] %s\n", desc); else { printf("  [FAIL] %s\n", desc); failures++; } } while (0)

int main(void) {
    printf("--- pump_output_state() ---\n");
    CHECK("water present, hyst ON -> ON", pump_output_state(1, 1) == 1);
    CHECK("water present, hyst OFF -> OFF", pump_output_state(0, 1) == 0);
    CHECK("water absent forces OFF", pump_output_state(1, 0) == 0);
    CHECK("water absent, already off", pump_output_state(0, 0) == 0);
    printf("\n");
    if (failures == 0) { printf("=== all tests passed ===\n"); return 0; }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
