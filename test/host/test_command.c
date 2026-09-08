/* Host-side unit tests for command_process() in command.c. */

#include <stdio.h>
#include "../../src/command.h"
#include "../../src/thresholds.h"

static int failures = 0;

#define CHECK(desc, cond)                                              \
    do {                                                               \
        if (cond) {                                                    \
            printf("  [OK] %s\n", desc);                               \
        } else {                                                       \
            printf("  [FAIL] %s\n", desc);                             \
            failures++;                                                \
        }                                                               \
    } while (0)

int main(void) {
    printf("--- command_process() ---\n");

    GreenhouseThresholds cfg = GREENHOUSE_DEFAULT_THRESHOLDS;

    CHECK("GET returns CMD_GET and does not modify cfg",
          command_process("GET", &cfg) == CMD_GET &&
          cfg.fan_on_decidegC == 260);

    CHECK("SET FAN_ON 275 is accepted",
          command_process("SET FAN_ON 275", &cfg) == CMD_SET_OK &&
          cfg.fan_on_decidegC == 275);

    CHECK("SET FAN_OFF 235 is accepted",
          command_process("SET FAN_OFF 235", &cfg) == CMD_SET_OK &&
          cfg.fan_off_decidegC == 235);

    CHECK("SET PUMP_ON 25 is accepted",
          command_process("SET PUMP_ON 25", &cfg) == CMD_SET_OK &&
          cfg.pump_on_percent == 25);

    CHECK("SET PUMP_OFF 55 is accepted",
          command_process("SET PUMP_OFF 55", &cfg) == CMD_SET_OK &&
          cfg.pump_off_percent == 55);

    CHECK("negative values parse correctly",
          command_process("SET FAN_OFF -50", &cfg) == CMD_SET_OK &&
          cfg.fan_off_decidegC == -50);

    /* Restore a known-good baseline before testing rejection: a rejected
     * command must leave cfg exactly as it was. */
    cfg = (GreenhouseThresholds)GREENHOUSE_DEFAULT_THRESHOLDS;

    CHECK("a SET that would invert the dead band is rejected, "
          "and cfg is left untouched",
          command_process("SET FAN_OFF 300", &cfg) == CMD_ERR_REJECTED &&
          cfg.fan_off_decidegC == 240);

    CHECK("a SET with no number is a bad-number error, cfg untouched",
          command_process("SET FAN_ON ", &cfg) == CMD_ERR_BAD_NUMBER &&
          cfg.fan_on_decidegC == 260);

    /* Same rejection path, the other three fields: gcov flagged these as
     * untested branches (each field has its own "if !looks_like_number"
     * check), not a guess. */
    CHECK("SET FAN_OFF with no number is a bad-number error",
          command_process("SET FAN_OFF ", &cfg) == CMD_ERR_BAD_NUMBER);

    CHECK("SET PUMP_ON with no number is a bad-number error",
          command_process("SET PUMP_ON ", &cfg) == CMD_ERR_BAD_NUMBER);

    CHECK("SET PUMP_OFF with no number is a bad-number error",
          command_process("SET PUMP_OFF ", &cfg) == CMD_ERR_BAD_NUMBER);

    CHECK("an unknown field name is an unknown-command error",
          command_process("SET FAN_MIDDLE 250", &cfg) == CMD_ERR_UNKNOWN);

    CHECK("garbage input is an unknown-command error, not a crash",
          command_process("asdkjfh", &cfg) == CMD_ERR_UNKNOWN);

    CHECK("empty line is an unknown-command error, not a crash",
          command_process("", &cfg) == CMD_ERR_UNKNOWN);

    cfg.fan_on_decidegC = 999; /* deliberately drifted from defaults */
    CHECK("RESET restores the factory defaults",
          command_process("RESET", &cfg) == CMD_RESET_OK &&
          cfg.fan_on_decidegC == 260);

    printf("\n");
    if (failures == 0) {
        printf("=== all tests passed ===\n");
        return 0;
    }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
