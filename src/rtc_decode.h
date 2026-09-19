#ifndef RTC_DECODE_H
#define RTC_DECODE_H

#include <stdint.h>

/* DS1307 stores time in BCD. Caller must mask non-digit bits first
 * (e.g. the 12/24h mode bit). */
uint8_t bcd_to_decimal(uint8_t bcd);

#endif
