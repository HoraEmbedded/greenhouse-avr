/* Host-side unit tests for the decision logic in hysteresis.c.
 *
 * Compiled with a plain host gcc, no AVR toolchain involved: hysteresis.c
 * has no I/O and no register access, so the exact same source that runs on
 * the ATmega2560 runs here unmodified.
 *
 * Run: make run   (from this directory)
 */

#include <stdio.h>
#include "../../src/hysteresis.h"
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

static const GreenhouseThresholds DEFAULTS = GREENHOUSE_DEFAULT_THRESHOLDS;

static void test_fan(void) {
    printf("--- Fan hysteresis (default thresholds: on 26.0C, off 24.0C) ---\n");

    CHECK("cold start, well below ON threshold stays off",
          fan_hysteresis(0, 200, &DEFAULTS) == 0);

    CHECK("crossing the ON threshold turns the fan on",
          fan_hysteresis(0, 260, &DEFAULTS) == 1);

    CHECK("just above ON threshold stays on",
          fan_hysteresis(1, 261, &DEFAULTS) == 1);

    CHECK("inside the dead band, fan was ON, stays ON",
          fan_hysteresis(1, 250, &DEFAULTS) == 1);

    CHECK("inside the dead band, fan was OFF, stays OFF",
          fan_hysteresis(0, 250, &DEFAULTS) == 0);

    CHECK("crossing the OFF threshold turns the fan off",
          fan_hysteresis(1, 240, &DEFAULTS) == 0);

    CHECK("well above OFF threshold, fan was off, stays off",
          fan_hysteresis(0, 245, &DEFAULTS) == 0);
}

static void test_pump(void) {
    printf("--- Pump hysteresis (default thresholds: on 30%%, off 60%%) ---\n");

    CHECK("dry soil, pump off, turns on",
          pump_hysteresis(0, 30, &DEFAULTS) == 1);

    CHECK("dry soil, pump already on, stays on",
          pump_hysteresis(1, 10, &DEFAULTS) == 1);

    CHECK("inside the dead band, pump was ON, stays ON",
          pump_hysteresis(1, 45, &DEFAULTS) == 1);

    CHECK("inside the dead band, pump was OFF, stays OFF",
          pump_hysteresis(0, 45, &DEFAULTS) == 0);

    CHECK("wet soil, pump on, turns off",
          pump_hysteresis(1, 60, &DEFAULTS) == 0);

    CHECK("saturated soil, pump off, stays off",
          pump_hysteresis(0, 90, &DEFAULTS) == 0);
}

static void test_no_chatter(void) {
    printf("--- No chatter around a single threshold ---\n");

    int16_t readings[] = {259, 260, 259, 260, 259, 260, 259, 260};
    uint8_t state = 0;
    int transitions = 0;
    uint8_t previous = state;

    for (size_t i = 0; i < sizeof(readings) / sizeof(readings[0]); i++) {
        state = fan_hysteresis(state, readings[i], &DEFAULTS);
        if (state != previous) transitions++;
        previous = state;
    }

    CHECK("oscillating around 260 causes at most one transition",
          transitions <= 1);
}

static void test_custom_thresholds(void) {
    /* Not the defaults: proves the function actually reads cfg instead of
     * having the old compile-time constants baked in somewhere. */
    printf("--- Custom thresholds are actually used, not just the defaults ---\n");

    GreenhouseThresholds tight = { .fan_on_decidegC = 300, .fan_off_decidegC = 290,
                                    .pump_on_percent = 20, .pump_off_percent = 25 };

    CHECK("custom fan ON at 30.0C: 29.5C stays off",
          fan_hysteresis(0, 295, &tight) == 0);
    CHECK("custom fan ON at 30.0C: 30.0C turns on",
          fan_hysteresis(0, 300, &tight) == 1);
    CHECK("default thresholds would have already turned this fan on, "
          "custom ones correctly do not",
          fan_hysteresis(0, 270, &tight) == 0);
}

int main(void) {
    test_fan();
    test_pump();
    test_no_chatter();
    test_custom_thresholds();

    printf("\n");
    if (failures == 0) {
        printf("=== all tests passed ===\n");
        return 0;
    }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
