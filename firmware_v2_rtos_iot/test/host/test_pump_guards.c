#include <assert.h>
#include <stdio.h>
#include "water_level.h"
#include "schedule.h"

static void test_pump_blocked_when_water_low(void) {
    assert(pump_output_state(1, 0) == 0);
    assert(pump_output_state(1, 1) == 1);
    assert(pump_output_state(0, 1) == 0);
}

static void test_daytime_window(void) {
    assert(is_daytime(12) == 1);
    assert(is_daytime(0)  == 0);
    assert(is_daytime(23) == 0);
}

int main(void) {
    test_pump_blocked_when_water_low();
    test_daytime_window();
    printf("test_pump_guards: OK\n");
    return 0;
}