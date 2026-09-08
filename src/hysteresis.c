#include "hysteresis.h"

/* The gap between an ON and OFF threshold is the dead band: without it, a
 * reading that hovers around a single value makes the relay chatter every
 * cycle. cfg's values are validated by thresholds_valid() before they can
 * ever reach here (see thresholds.c and command.c), so a live dead band
 * is guaranteed, not just assumed. */

uint8_t fan_hysteresis(uint8_t current_state, int16_t temp_decidegC,
                        const GreenhouseThresholds *cfg) {
    if (temp_decidegC >= cfg->fan_on_decidegC) return 1;
    if (temp_decidegC <= cfg->fan_off_decidegC) return 0;
    return current_state; /* inside the dead band: no change */
}

uint8_t pump_hysteresis(uint8_t current_state, int8_t soil_percent,
                         const GreenhouseThresholds *cfg) {
    if (soil_percent <= cfg->pump_on_percent) return 1;
    if (soil_percent >= cfg->pump_off_percent) return 0;
    return current_state;
}
