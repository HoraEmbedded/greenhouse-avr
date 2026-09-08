#ifndef THRESHOLDS_H
#define THRESHOLDS_H

#include <stdint.h>

/* Configurable operating thresholds. int16_t fields first, then int8_t:
 * on AVR-GCC this needs no padding (checked with sizeof() in the host
 * tests, not just assumed), which matters once this struct is written
 * byte-for-byte into EEPROM. */
typedef struct {
    int16_t fan_on_decidegC;
    int16_t fan_off_decidegC;
    int8_t  pump_on_percent;
    int8_t  pump_off_percent;
} GreenhouseThresholds;

/* Factory defaults -- identical to the fixed values the firmware used
 * before thresholds became configurable, so existing behaviour on a
 * fresh chip does not change. */
#define GREENHOUSE_DEFAULT_THRESHOLDS \
    { .fan_on_decidegC = 260, .fan_off_decidegC = 240, \
      .pump_on_percent = 30,  .pump_off_percent = 60 }

/* Sensor range: the DHT22 datasheet bounds, not an arbitrary guess. */
#define THRESHOLD_TEMP_MIN_DECIDEG (-400)
#define THRESHOLD_TEMP_MAX_DECIDEG   800

/* Returns 1 if the configuration is internally consistent and within
 * sensor range, 0 otherwise. Pure function: no I/O, testable on the host.
 *
 * "Internally consistent" means the ON and OFF thresholds do not cross:
 * a fan ON at or below its OFF value (or a pump ON at or above its OFF
 * value) would collapse or invert the dead band, and the relay would
 * chatter on every single reading -- precisely what hysteresis exists to
 * prevent. Rejecting that combination here means it can never reach the
 * actuators, however it was entered. */
uint8_t thresholds_valid(const GreenhouseThresholds *t);

#endif
