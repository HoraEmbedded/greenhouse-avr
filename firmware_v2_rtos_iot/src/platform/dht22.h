#ifndef PLATFORM_DHT22_H
#define PLATFORM_DHT22_H

#include <stdbool.h>

void dht22_init(void);
bool dht22_read(float *temperature_c, float *humidity_pct);

#endif