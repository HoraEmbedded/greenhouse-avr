#ifndef TELEMETRY_HUMAN_H
#define TELEMETRY_HUMAN_H

#include <stddef.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

int telemetry_to_human(const Telemetry_t *t, char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif