#ifndef WATER_LEVEL_H
#define WATER_LEVEL_H

#include <stdint.h>

/* Forces pump OFF if water isn't confirmed present, overriding
 * hysteresis (broken/disconnected wire reads as "absent" -- fails
 * safe). */
uint8_t pump_output_state(uint8_t hysteresis_pump_state, uint8_t water_present);

#endif
