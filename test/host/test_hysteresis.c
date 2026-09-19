#include <stdio.h>
#include "../../src/hysteresis.h"
#include "../../src/thresholds.h"

static int failures = 0;
#define CHECK(desc, cond) do { if (cond) printf("  [OK] %s\n", desc); else { printf("  [FAIL] %s\n", desc); failures++; } } while (0)

static const GreenhouseThresholds DEFAULTS = GREENHOUSE_DEFAULT_THRESHOLDS;

static void test_fan(void) {
    printf("--- fan_hysteresis() ---\n");
    CHECK("cold stays off", fan_hysteresis(0, 200, &DEFAULTS) == 0);
    CHECK("crosses ON at 26.0", fan_hysteresis(0, 260, &DEFAULTS) == 1);
    CHECK("above ON stays on", fan_hysteresis(1, 261, &DEFAULTS) == 1);
    CHECK("dead band, was ON", fan_hysteresis(1, 250, &DEFAULTS) == 1);
    CHECK("dead band, was OFF", fan_hysteresis(0, 250, &DEFAULTS) == 0);
    CHECK("crosses OFF at 24.0", fan_hysteresis(1, 240, &DEFAULTS) == 0);
    CHECK("above OFF, was off", fan_hysteresis(0, 245, &DEFAULTS) == 0);
}

static void test_pump(void) {
    printf("--- pump_hysteresis() ---\n");
    CHECK("dry turns on", pump_hysteresis(0, 30, &DEFAULTS) == 1);
    CHECK("dry stays on", pump_hysteresis(1, 10, &DEFAULTS) == 1);
    CHECK("dead band, was ON", pump_hysteresis(1, 45, &DEFAULTS) == 1);
    CHECK("dead band, was OFF", pump_hysteresis(0, 45, &DEFAULTS) == 0);
    CHECK("wet turns off", pump_hysteresis(1, 60, &DEFAULTS) == 0);
    CHECK("saturated stays off", pump_hysteresis(0, 90, &DEFAULTS) == 0);
}

static void test_no_chatter(void) {
    printf("--- no chatter around threshold ---\n");
    int16_t readings[] = {259, 260, 259, 260, 259, 260, 259, 260};
    uint8_t state = 0, previous = 0;
    int transitions = 0;
    for (size_t i = 0; i < sizeof(readings) / sizeof(readings[0]); i++) {
        state = fan_hysteresis(state, readings[i], &DEFAULTS);
        if (state != previous) transitions++;
        previous = state;
    }
    CHECK("<=1 transition oscillating at 260", transitions <= 1);
}

static void test_custom_thresholds(void) {
    printf("--- custom (non-default) thresholds ---\n");
    GreenhouseThresholds tight = { .fan_on_decidegC = 300, .fan_off_decidegC = 290,
                                    .pump_on_percent = 20, .pump_off_percent = 25 };
    CHECK("29.5C stays off (custom ON=30.0)", fan_hysteresis(0, 295, &tight) == 0);
    CHECK("30.0C turns on (custom)", fan_hysteresis(0, 300, &tight) == 1);
    CHECK("27.0C stays off (would be ON with defaults)", fan_hysteresis(0, 270, &tight) == 0);
}

int main(void) {
    test_fan();
    test_pump();
    test_no_chatter();
    test_custom_thresholds();
    printf("\n");
    if (failures == 0) { printf("=== all tests passed ===\n"); return 0; }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
