#include "command.h"

/* No <string.h>/sscanf on purpose: this project hand-writes its own
 * primitives elsewhere (uart_send_int in main.c does the same for the
 * opposite direction), and a hand-rolled parser stays a fraction of the
 * Flash cost that pulling in scanf-family code would add. */

static uint8_t starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++;
        prefix++;
    }
    return 1;
}

static uint8_t str_equal(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return *a == *b;
}

static uint8_t looks_like_number(const char *s) {
    if (*s == '-') s++;
    return (*s >= '0' && *s <= '9');
}

static int16_t parse_int(const char *s) {
    int16_t sign = 1;
    int16_t value = 0;

    if (*s == '-') {
        sign = -1;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        value = (int16_t)(value * 10 + (*s - '0'));
        s++;
    }
    return (int16_t)(value * sign);
}

CommandResult command_process(const char *line, GreenhouseThresholds *cfg) {
    if (str_equal(line, "GET")) return CMD_GET;

    if (str_equal(line, "RESET")) {
        GreenhouseThresholds defaults = GREENHOUSE_DEFAULT_THRESHOLDS;
        *cfg = defaults;
        return CMD_RESET_OK;
    }

    if (starts_with(line, "SET ")) {
        const char *rest = line + 4;
        const char *value_str;
        GreenhouseThresholds candidate = *cfg;

        if (starts_with(rest, "FAN_ON ")) {
            value_str = rest + 7;
            if (!looks_like_number(value_str)) return CMD_ERR_BAD_NUMBER;
            candidate.fan_on_decidegC = parse_int(value_str);
        } else if (starts_with(rest, "FAN_OFF ")) {
            value_str = rest + 8;
            if (!looks_like_number(value_str)) return CMD_ERR_BAD_NUMBER;
            candidate.fan_off_decidegC = parse_int(value_str);
        } else if (starts_with(rest, "PUMP_ON ")) {
            value_str = rest + 8;
            if (!looks_like_number(value_str)) return CMD_ERR_BAD_NUMBER;
            candidate.pump_on_percent = (int8_t)parse_int(value_str);
        } else if (starts_with(rest, "PUMP_OFF ")) {
            value_str = rest + 9;
            if (!looks_like_number(value_str)) return CMD_ERR_BAD_NUMBER;
            candidate.pump_off_percent = (int8_t)parse_int(value_str);
        } else {
            return CMD_ERR_UNKNOWN;
        }

        if (!thresholds_valid(&candidate)) return CMD_ERR_REJECTED;

        *cfg = candidate;
        return CMD_SET_OK;
    }

    return CMD_ERR_UNKNOWN;
}
