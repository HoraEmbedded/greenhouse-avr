#include "thresholds.h"

uint8_t thresholds_valid(const GreenhouseThresholds *t) {
    if (t->fan_on_decidegC <= t->fan_off_decidegC) return 0;
    if (t->fan_on_decidegC < THRESHOLD_TEMP_MIN_DECIDEG ||
        t->fan_on_decidegC > THRESHOLD_TEMP_MAX_DECIDEG) return 0;
    if (t->fan_off_decidegC < THRESHOLD_TEMP_MIN_DECIDEG ||
        t->fan_off_decidegC > THRESHOLD_TEMP_MAX_DECIDEG) return 0;

    if (t->pump_on_percent >= t->pump_off_percent) return 0;
    if (t->pump_on_percent < 0 || t->pump_on_percent > 100) return 0;
    if (t->pump_off_percent < 0 || t->pump_off_percent > 100) return 0;

    return 1;
}

/* NOTE: two branches above (fan_off > MAX, pump_off < 0) are
 * unreachable by construction -- see docs, not a test gap. */
