#ifndef NETWORK_PUBLISHER_H
#define NETWORK_PUBLISHER_H

#include <stdbool.h>
#include "config.h"

bool publisher_init(void);
bool publisher_is_ready(void);
bool publisher_publish(const Telemetry_t *t);

#endif