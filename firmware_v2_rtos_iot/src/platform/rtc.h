#ifndef PLATFORM_RTC_H
#define PLATFORM_RTC_H

#include <stdint.h>
#include <stdbool.h>

void rtc_init(void);
bool rtc_read_hour(uint8_t *hour_out);

#endif