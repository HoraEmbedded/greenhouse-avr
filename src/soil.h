#ifndef SOIL_H
#define SOIL_H

#include <stdint.h>

/* Calibration points, in raw ADC counts (0-1023).
 *
 * These defaults (0 -> 0 %, 1023 -> 100 %) match the Wokwi potentiometer
 * used in simulation, which is a linear stand-in and nothing more.
 *
 * A REAL capacitive soil probe is NOT linear across its full range and is
 * often the OPPOSITE polarity: many boards read a HIGHER raw value when the
 * soil is DRIER. Before trusting this on real hardware, measure both ends
 * yourself: read the raw ADC value with the probe in open air (call it
 * SOIL_ADC_AT_0_PERCENT) and with the probe fully submerged in water (call
 * it SOIL_ADC_AT_100_PERCENT), then replace the two constants below. If
 * your sensor is inverted, AT_0_PERCENT will simply be the larger number,
 * and the formula still works -- it does not assume which one is bigger.
 */
#ifndef SOIL_ADC_AT_0_PERCENT
#define SOIL_ADC_AT_0_PERCENT    0
#endif
#ifndef SOIL_ADC_AT_100_PERCENT
#define SOIL_ADC_AT_100_PERCENT  1023
#endif

/* Maps a raw ADC reading to a 0-100 percent value using the two
 * calibration points above, clamped to [0, 100] so a sensor drifting past
 * its calibrated range never reports outside the meaningful scale. */
int8_t soil_percent_from_raw(uint16_t raw);

#endif
