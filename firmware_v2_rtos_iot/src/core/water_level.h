#ifndef WATER_LEVEL_H
#define WATER_LEVEL_H

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/* Forces pump OFF if water isn't confirmed present, overriding
 * hysteresis (broken/disconnected wire reads as "absent" -- fails
 * safe). */
uint8_t pump_output_state(uint8_t hysteresis_pump_state, uint8_t water_present);

#ifdef __cplusplus
}
#endif

#endif
