#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdbool.h>

void i2c_bus_init(SemaphoreHandle_t mutex);
bool i2c_bus_lock(TickType_t timeout_ticks);
void i2c_bus_unlock(void);

#endif