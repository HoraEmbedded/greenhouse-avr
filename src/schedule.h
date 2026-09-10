#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>

/* Irrigation window, 24-hour clock. Not arbitrary: watering foliage
 * that stays wet overnight (no sun, lower temperature, slower
 * evaporation) promotes fungal growth in a real greenhouse -- this is
 * an agronomic constraint, not just a schedule preference. */
#define DAYTIME_START_HOUR 6
#define DAYTIME_END_HOUR   20

/* True if hour (0-23) falls within [DAYTIME_START_HOUR,
 * DAYTIME_END_HOUR). Pure function: no I/O, testable on the host
 * (test/host/test_schedule.c). */
uint8_t is_daytime(uint8_t hour);

#endif
