#include <stdio.h>
#include "../../src/command.h"
#include "../../src/thresholds.h"

static int failures = 0;
#define CHECK(desc, cond) do { if (cond) printf("  [OK] %s\n", desc); else { printf("  [FAIL] %s\n", desc); failures++; } } while (0)

int main(void) {
    printf("--- command_process() ---\n");
    GreenhouseThresholds cfg = GREENHOUSE_DEFAULT_THRESHOLDS;

    CHECK("GET doesn't modify cfg", command_process("GET", &cfg) == CMD_GET && cfg.fan_on_decidegC == 260);
    CHECK("SET FAN_ON 275", command_process("SET FAN_ON 275", &cfg) == CMD_SET_OK && cfg.fan_on_decidegC == 275);
    CHECK("SET FAN_OFF 235", command_process("SET FAN_OFF 235", &cfg) == CMD_SET_OK && cfg.fan_off_decidegC == 235);
    CHECK("SET PUMP_ON 25", command_process("SET PUMP_ON 25", &cfg) == CMD_SET_OK && cfg.pump_on_percent == 25);
    CHECK("SET PUMP_OFF 55", command_process("SET PUMP_OFF 55", &cfg) == CMD_SET_OK && cfg.pump_off_percent == 55);
    CHECK("negative value parses", command_process("SET FAN_OFF -50", &cfg) == CMD_SET_OK && cfg.fan_off_decidegC == -50);

    cfg = (GreenhouseThresholds)GREENHOUSE_DEFAULT_THRESHOLDS;

    CHECK("invalid SET rejected, cfg unchanged", command_process("SET FAN_OFF 300", &cfg) == CMD_ERR_REJECTED && cfg.fan_off_decidegC == 240);
    CHECK("SET FAN_ON no number", command_process("SET FAN_ON ", &cfg) == CMD_ERR_BAD_NUMBER && cfg.fan_on_decidegC == 260);
    CHECK("SET FAN_OFF no number", command_process("SET FAN_OFF ", &cfg) == CMD_ERR_BAD_NUMBER);
    CHECK("SET PUMP_ON no number", command_process("SET PUMP_ON ", &cfg) == CMD_ERR_BAD_NUMBER);
    CHECK("SET PUMP_OFF no number", command_process("SET PUMP_OFF ", &cfg) == CMD_ERR_BAD_NUMBER);
    CHECK("unknown field", command_process("SET FAN_MIDDLE 250", &cfg) == CMD_ERR_UNKNOWN);
    CHECK("garbage input", command_process("asdkjfh", &cfg) == CMD_ERR_UNKNOWN);
    CHECK("empty line", command_process("", &cfg) == CMD_ERR_UNKNOWN);

    cfg.fan_on_decidegC = 999;
    CHECK("RESET restores defaults", command_process("RESET", &cfg) == CMD_RESET_OK && cfg.fan_on_decidegC == 260);

    printf("\n");
    if (failures == 0) { printf("=== all tests passed ===\n"); return 0; }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
