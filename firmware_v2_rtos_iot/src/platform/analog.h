#ifndef PLATFORM_ANALOG_H
#define PLATFORM_ANALOG_H

#include <stdint.h>
#include <stdbool.h>

void     analog_init(void);
uint16_t analog_read_soil_raw(void);
bool     water_level_ok(void);

#endif