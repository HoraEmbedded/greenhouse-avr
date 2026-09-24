#include <assert.h>
#include <stdio.h>
#include "hysteresis.h"
#include "thresholds.h"

static GreenhouseThresholds cfg(void) {
    GreenhouseThresholds c = {
        .fan_on_decidegC  = 260,
        .fan_off_decidegC = 240,
        .pump_on_percent  = 30,
        .pump_off_percent = 60,
    };
    return c;
}

static void test_fan_below_on_stays_off(void) {
    GreenhouseThresholds c = cfg();
    assert(fan_hysteresis(0, 250, &c) == 0);
}

static void test_fan_above_on_turns_on(void) {
    GreenhouseThresholds c = cfg();
    assert(fan_hysteresis(0, 270, &c) == 1);
}

static void test_fan_deadband_holds(void) {
    GreenhouseThresholds c = cfg();
    assert(fan_hysteresis(1, 250, &c) == 1);
    assert(fan_hysteresis(0, 250, &c) == 0);
}

static void test_pump_below_on_turns_on(void) {
    GreenhouseThresholds c = cfg();
    assert(pump_hysteresis(0, 20, &c) == 1);
}

static void test_pump_above_off_turns_off(void) {
    GreenhouseThresholds c = cfg();
    assert(pump_hysteresis(1, 70, &c) == 0);
}

int main(void) {
    test_fan_below_on_stays_off();
    test_fan_above_on_turns_on();
    test_fan_deadband_holds();
    test_pump_below_on_turns_on();
    test_pump_above_off_turns_off();
    printf("test_hysteresis: OK\n");
    return 0;
}