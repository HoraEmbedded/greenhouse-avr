#ifndef HYSTERESIS_H
#define HYSTERESIS_H

#include <stdint.h>
#include "thresholds.h"

/* Decision logic only -- no I/O, no AVR register access. This is what makes
 * it testable with a plain host compiler (see test/host/test_hysteresis.c),
 * the same principle as separating a target's logic from its hardware in a
 * larger embedded project: verify the DECISION on its own, independently of
 * what carries it out.
 *
 * Thresholds are now passed in explicitly (cfg) rather than fixed at
 * compile time, so they can be changed at runtime and persisted to EEPROM
 * -- see thresholds.h and eeprom_config.h. */

uint8_t fan_hysteresis(uint8_t current_state, int16_t temp_decidegC,
                        const GreenhouseThresholds *cfg);

uint8_t pump_hysteresis(uint8_t current_state, int8_t soil_percent,
                         const GreenhouseThresholds *cfg);

#endif
