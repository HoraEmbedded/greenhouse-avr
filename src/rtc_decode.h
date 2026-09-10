#ifndef RTC_DECODE_H
#define RTC_DECODE_H

#include <stdint.h>

/* The DS1307 stores time fields in BCD (binary-coded decimal): each
 * nibble is one decimal digit, not a binary value -- register 0x02 for
 * "23 hours" holds 0x23, not 23. Pure function: no I2C, no register
 * access, testable on the host (test/host/test_rtc_decode.c).
 *
 * Callers must mask out any non-digit bits first (e.g. the DS1307
 * hours register's 12/24-hour mode bit) -- this function assumes both
 * nibbles are already genuine BCD digits. */
uint8_t bcd_to_decimal(uint8_t bcd);

#endif
