#ifndef PLATFORM_SENSORS_H
#define PLATFORM_SENSORS_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "config.h"

void sensors_init(SemaphoreHandle_t i2c_mutex);
void sensors_read_all(SensorData_t *out, uint8_t *dht_failures);

void actuators_init(void);
void actuators_apply(const ActuatorState_t *state);

#endif