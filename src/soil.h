#ifndef SOIL_H
#define SOIL_H

#include <stdint.h>

/* Calibration points, raw ADC counts. Defaults match the Wokwi pot
 * (linear, full range). A real capacitive probe needs its own
 * calibration and may be inverted -- re-measure both ends. */
#ifndef SOIL_ADC_AT_0_PERCENT
#define SOIL_ADC_AT_0_PERCENT    0
#endif
#ifndef SOIL_ADC_AT_100_PERCENT
#define SOIL_ADC_AT_100_PERCENT  1023
#endif

/* Maps raw ADC to 0-100%, clamped. */
int8_t soil_percent_from_raw(uint16_t raw);

#endif
