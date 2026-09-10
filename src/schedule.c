#include "schedule.h"

uint8_t is_daytime(uint8_t hour) {
    return hour >= DAYTIME_START_HOUR && hour < DAYTIME_END_HOUR;
}
