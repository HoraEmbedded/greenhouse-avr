#include "rtc.h"
#include <Wire.h>
#include "i2c_bus.h"
#include "rtc_decode.h"

#define DS1307_ADDR 0x68
#define I2C_LOCK_TIMEOUT_MS 100

void rtc_init(void) {}

bool rtc_read_hour(uint8_t *hour_out)
{
    if (!i2c_bus_lock(pdMS_TO_TICKS(I2C_LOCK_TIMEOUT_MS))) return false;

    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(0x02);
    if (Wire.endTransmission(false) != 0) {
        i2c_bus_unlock();
        return false;
    }
    if (Wire.requestFrom(DS1307_ADDR, 1) != 1) {
        i2c_bus_unlock();
        return false;
    }
    uint8_t raw = Wire.read();
    i2c_bus_unlock();

    *hour_out = bcd_to_decimal(raw & 0x3F);
    return true;
}