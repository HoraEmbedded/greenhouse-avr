#include "telemetry_2.h"
#include <stdio.h>

int telemetry_to_human(const Telemetry_t *t, char *buf, size_t buf_size)
{
    int n = snprintf(buf, buf_size,
        "T:%.0fC | Air:%.0f%% | Soil:%.0f%% | Fan:%s | Pump:%s | Water:%s | Day:%s | Fault:%s",
        t->sensors.temperature_c,
        t->sensors.humidity_pct,
        t->sensors.soil_moisture_pct,
        t->actuators.fan_on       ? "ON"  : "OFF",
        t->actuators.pump_on      ? "ON"  : "OFF",
        t->sensors.water_level_ok ? "OK"  : "LOW",
        t->sensors.is_daytime     ? "YES" : "NO",
        t->actuators.sensor_fault ? "YES" : "NO");

    if (n < 0 || (size_t)n >= buf_size) return -1;
    return n;
}