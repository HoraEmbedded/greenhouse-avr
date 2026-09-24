#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "fault_handling.h"

static void test_degrade_forces_fan_on_at_threshold(void) {
    bool state = degraded_fan_state(DHT_FAILURE_SAFETY_THRESHOLD, false);
    assert(state == true);
}

static void test_below_threshold_keeps_state(void) {
    assert(degraded_fan_state(0, false) == false);
    assert(degraded_fan_state(0, true)  == true);
}

int main(void) {
    test_degrade_forces_fan_on_at_threshold();
    test_below_threshold_keeps_state();
    printf("test_fault_handling: OK\n");
    return 0;
}