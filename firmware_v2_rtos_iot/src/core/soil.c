#include "soil.h"

int8_t soil_percent_from_raw(uint16_t raw) {
    int32_t span = (int32_t)SOIL_ADC_AT_100_PERCENT - (int32_t)SOIL_ADC_AT_0_PERCENT;
    int32_t pct = ((int32_t)raw - (int32_t)SOIL_ADC_AT_0_PERCENT) * 100L / span;

    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return (int8_t)pct;
}
