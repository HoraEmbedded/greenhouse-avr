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

/* Commands: GET, RESET, SET FAN_ON|FAN_OFF|PUMP_ON|PUMP_OFF <n>.
 * Rejects invalid config; leaves *cfg unchanged on error. */
CommandResult command_process(const char *line, GreenhouseThresholds *cfg);

#endif
