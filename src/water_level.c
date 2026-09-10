#include "water_level.h"

uint8_t pump_output_state(uint8_t hysteresis_pump_state, uint8_t water_present) {
    if (!water_present) return 0;
    return hysteresis_pump_state;
}
