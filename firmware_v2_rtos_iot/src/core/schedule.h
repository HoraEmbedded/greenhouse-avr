#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>

/* Daytime-only irrigation window (fungal growth risk at night). */
#define DAYTIME_START_HOUR 6
#define DAYTIME_END_HOUR   20

uint8_t is_daytime(uint8_t hour);

#endif
