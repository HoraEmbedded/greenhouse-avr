#ifndef COMMAND_H
#define COMMAND_H

#include "thresholds.h"

typedef enum {
    CMD_GET,
    CMD_SET_OK,
    CMD_RESET_OK,
    CMD_ERR_UNKNOWN,
    CMD_ERR_BAD_NUMBER,
    CMD_ERR_REJECTED
} CommandResult;

/* Parses ONE line (no trailing newline, main.c strips it) and, for
 * SET/RESET, updates *cfg in place -- but only if the resulting
 * configuration passes thresholds_valid(). A rejected or malformed
 * command leaves *cfg untouched. Pure function: no I/O, no EEPROM access,
 * testable on the host (test/host/test_command.c).
 *
 * Recognised commands:
 *   GET
 *   RESET
 *   SET FAN_ON <tenths of degC>
 *   SET FAN_OFF <tenths of degC>
 *   SET PUMP_ON <percent>
 *   SET PUMP_OFF <percent>
 */
CommandResult command_process(const char *line, GreenhouseThresholds *cfg);

#endif
