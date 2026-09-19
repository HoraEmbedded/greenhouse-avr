#include "hysteresis.h"

uint8_t fan_hysteresis(uint8_t current_state, int16_t temp_decidegC,
                        const GreenhouseThresholds *cfg) {
    if (temp_decidegC >= cfg->fan_on_decidegC) return 1;
    if (temp_decidegC <= cfg->fan_off_decidegC) return 0;
    return current_state;
}

uint8_t pump_hysteresis(uint8_t current_state, int8_t soil_percent,
                         const GreenhouseThresholds *cfg) {
    if (soil_percent <= cfg->pump_on_percent) return 1;
    if (soil_percent >= cfg->pump_off_percent) return 0;
    return current_state;
}
