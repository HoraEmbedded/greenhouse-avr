#include <assert.h>
#include <stdio.h>
#include <stdbool.h> 
#include "fault_handling.h"

static void test_degrade_forces_fan_on_after_threshold(void) {
    bool state = false;
    for (uint8_t i = 0; i < DHT_FAILURE_SAFETY_THRESHOLD; i++) {
        state = degraded_fan_state(i, state);
    }
    assert(state == true);
}

static void test_below_threshold_keeps_state(void) {
    assert(degraded_fan_state(0, false) == false);
}

int main(void) {
    test_degrade_forces_fan_on_after_threshold();
    test_below_threshold_keeps_state();
    printf("test_fault_handling: OK\n");
    return 0;
}