#ifndef THRESHOLDS_H
#define THRESHOLDS_H

#include <stdint.h>

/* int16 fields first: avoids AVR-GCC padding (struct is written
 * byte-for-byte to EEPROM). */
typedef struct {
    int16_t fan_on_decidegC;
    int16_t fan_off_decidegC;
    int8_t  pump_on_percent;
    int8_t  pump_off_percent;
} GreenhouseThresholds;

#define GREENHOUSE_DEFAULT_THRESHOLDS \
    { .fan_on_decidegC = 260, .fan_off_decidegC = 240, \
      .pump_on_percent = 30,  .pump_off_percent = 60 }

#define THRESHOLD_TEMP_MIN_DECIDEG (-400)
#define THRESHOLD_TEMP_MAX_DECIDEG   800

#ifdef __cplusplus
extern "C" {
#endif

/* Range + ordering check. Pure, host-testable. */
uint8_t thresholds_valid(const GreenhouseThresholds *t);

#ifdef __cplusplus
}
#endif

#endif
