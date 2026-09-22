#include <stdio.h>
#include "../../src/thresholds.h"

static int failures = 0;
#define CHECK(desc, cond) do { if (cond) printf("  [OK] %s\n", desc); else { printf("  [FAIL] %s\n", desc); failures++; } } while (0)

int main(void) {
    printf("--- thresholds_valid() ---\n");

    GreenhouseThresholds defaults = GREENHOUSE_DEFAULT_THRESHOLDS;
    CHECK("defaults valid", thresholds_valid(&defaults) == 1);
    CHECK("no struct padding", sizeof(GreenhouseThresholds) == 6);

    GreenhouseThresholds fan_inverted = defaults;
    fan_inverted.fan_on_decidegC = 240;
    fan_inverted.fan_off_decidegC = 260;
    CHECK("fan on<=off rejected", thresholds_valid(&fan_inverted) == 0);

    GreenhouseThresholds fan_equal = defaults;
    fan_equal.fan_on_decidegC = 250;
    fan_equal.fan_off_decidegC = 250;
    CHECK("fan on==off rejected", thresholds_valid(&fan_equal) == 0);

    GreenhouseThresholds pump_inverted = defaults;
    pump_inverted.pump_on_percent = 60;
    pump_inverted.pump_off_percent = 30;
    CHECK("pump on>=off rejected", thresholds_valid(&pump_inverted) == 0);

    GreenhouseThresholds pump_oor = defaults;
    pump_oor.pump_off_percent = 120;
    CHECK("pump >100% rejected", thresholds_valid(&pump_oor) == 0);

    GreenhouseThresholds pump_neg = defaults;
    pump_neg.pump_on_percent = -5;
    CHECK("negative pump % rejected", thresholds_valid(&pump_neg) == 0);

    GreenhouseThresholds temp_hot = defaults;
    temp_hot.fan_on_decidegC = 900;
    CHECK("temp above sensor range rejected", thresholds_valid(&temp_hot) == 0);

    GreenhouseThresholds temp_cold = defaults;
    temp_cold.fan_off_decidegC = -500;
    CHECK("temp below sensor range rejected", thresholds_valid(&temp_cold) == 0);

    GreenhouseThresholds fan_on_cold = defaults;
    fan_on_cold.fan_on_decidegC = -410;
    fan_on_cold.fan_off_decidegC = -420;
    CHECK("fan_on below range rejected (real branch)", thresholds_valid(&fan_on_cold) == 0);

    GreenhouseThresholds pump_on_high = defaults;
    pump_on_high.pump_on_percent = 150;
    pump_on_high.pump_off_percent = 200;
    CHECK("pump_on >100 rejected (real branch)", thresholds_valid(&pump_on_high) == 0);

    printf("\n");
    if (failures == 0) { printf("=== all tests passed ===\n"); return 0; }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
