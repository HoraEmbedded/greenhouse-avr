#include "sensors.h"
#include <Arduino.h>
#include "board.h"
#include "i2c_bus.h"
#include "dht22.h"
#include "analog.h"
#include "rtc.h"
#include "soil.h"
#include "schedule.h"

#define DHT_FAILURE_MARKER (-100.0f)

void sensors_init(SemaphoreHandle_t i2c_mutex)
{
    i2c_bus_init(i2c_mutex);
    dht22_init();
    rtc_init();
    analog_init();


}

void sensors_read_all(SensorData_t *out, uint8_t *dht_failures)
{
    float t = DHT_FAILURE_MARKER;
    float h = 0.0f;
    if (dht22_read(&t, &h)) {
        *dht_failures = 0;
    } else {
        if (*dht_failures < 255) (*dht_failures)++;
    }

    uint16_t soil_raw = analog_read_soil_raw();

    uint8_t hour = 0;
    bool rtc_ok = rtc_read_hour(&hour);

    out->temperature_c    = t;
    out->humidity_pct     = h;
    out->soil_moisture_pct = (float)soil_percent_from_raw(soil_raw);
    out->water_level_ok   = water_level_ok();
    out->is_daytime       = rtc_ok ? is_daytime(hour) : true;
    out->timestamp_ms     = millis();
}

void actuators_init(void)
{
    pinMode(PIN_FAN_LED,  OUTPUT);
    pinMode(PIN_PUMP_LED, OUTPUT);
    digitalWrite(PIN_FAN_LED,  LOW);
    digitalWrite(PIN_PUMP_LED, LOW);
}

void actuators_apply(const ActuatorState_t *state)
{
    digitalWrite(PIN_FAN_LED,  state->fan_on  ? HIGH : LOW);
    digitalWrite(PIN_PUMP_LED, state->pump_on ? HIGH : LOW);
}