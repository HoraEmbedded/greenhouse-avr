#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "telemetry_json.h"

static void test_basic_serialization(void) {
    Telemetry_t t = {0};
    t.sensors.timestamp_ms      = 12345;
    t.sensors.temperature_c     = 23.5f;
    t.sensors.humidity_pct      = 45.0f;
    t.sensors.soil_moisture_pct = 35.0f;
    t.sensors.water_level_ok    = true;
    t.sensors.is_daytime        = true;
    t.actuators.fan_on          = false;
    t.actuators.pump_on         = true;
    t.actuators.sensor_fault    = false;
    t.sequence                  = 7;

    char buf[256];
    int n = telemetry_to_json(&t, buf, sizeof(buf));
    assert(n > 0);
    assert(strstr(buf, "\"ts\":12345")     != NULL);
    assert(strstr(buf, "\"seq\":7")        != NULL);
    assert(strstr(buf, "\"temp_c\":23.50") != NULL);
    assert(strstr(buf, "\"soil_pct\":35.0")!= NULL);
    assert(strstr(buf, "\"pump\":true")    != NULL);
    assert(strstr(buf, "\"fan\":false")    != NULL);
}

static void test_truncation_detected(void) {
    Telemetry_t t = {0};
    char buf[10];
    int n = telemetry_to_json(&t, buf, sizeof(buf));
    assert(n == -1);
}

int main(void) {
    test_basic_serialization();
    test_truncation_detected();
    printf("test_telemetry_json: OK\n");
    return 0;
}