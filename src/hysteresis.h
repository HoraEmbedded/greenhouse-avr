#ifndef HYSTERESIS_H
#define HYSTERESIS_H

#include <stdint.h>
#include "thresholds.h"

/* Pure decision logic, no I/O. Thresholds passed by reference so they
 * can change at runtime (EEPROM-backed). */
uint8_t fan_hysteresis(uint8_t current_state, int16_t temp_decidegC,
                        const GreenhouseThresholds *cfg);
uint8_t pump_hysteresis(uint8_t current_state, int8_t soil_percent,
                         const GreenhouseThresholds *cfg);

#endif
