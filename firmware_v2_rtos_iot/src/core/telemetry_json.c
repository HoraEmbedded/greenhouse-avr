#include "telemetry_json.h"
#include <stdio.h>

int telemetry_to_json(const Telemetry_t *t, char *buf, size_t buf_size)
{
    int n = snprintf(buf, buf_size,
        "{\"ts\":%lu,\"seq\":%lu,"
        "\"temp_c\":%.2f,\"hum_pct\":%.2f,\"soil_pct\":%.1f,"
        "\"fan\":%s,\"pump\":%s,\"water_ok\":%s,\"day\":%s,\"fault\":%s}",
        (unsigned long)t->sensors.timestamp_ms,
        (unsigned long)t->sequence,
        t->sensors.temperature_c,
        t->sensors.humidity_pct,
        t->sensors.soil_moisture_pct,
        t->actuators.fan_on       ? "true" : "false",
        t->actuators.pump_on      ? "true" : "false",
        t->sensors.water_level_ok ? "true" : "false",
        t->sensors.is_daytime     ? "true" : "false",
        t->actuators.sensor_fault ? "true" : "false");

    if (n < 0 || (size_t)n >= buf_size) return -1;
    return n;
}