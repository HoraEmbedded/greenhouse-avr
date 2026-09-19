#include "fault_handling.h"

uint8_t degraded_fan_state(uint8_t consecutive_failures,
                            uint8_t last_known_fan_state) {
    if (consecutive_failures >= DHT_FAILURE_SAFETY_THRESHOLD) return 1;
    return last_known_fan_state;
}
