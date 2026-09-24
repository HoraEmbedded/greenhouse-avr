#include "i2c_bus.h"
#include <Wire.h>
#include "board.h"

static SemaphoreHandle_t s_mutex = nullptr;

void i2c_bus_init(SemaphoreHandle_t mutex)
{
    s_mutex = mutex;
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQ_HZ);
}

bool i2c_bus_lock(TickType_t timeout_ticks)
{
    if (!s_mutex) return false;
    return xSemaphoreTake(s_mutex, timeout_ticks) == pdTRUE;
}

void i2c_bus_unlock(void)
{
    if (s_mutex) xSemaphoreGive(s_mutex);
}