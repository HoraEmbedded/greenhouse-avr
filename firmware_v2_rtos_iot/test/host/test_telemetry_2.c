#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "telemetry_2.h"

static void test_basic_format(void) {
    Telemetry_t t = {0};
    t.sensors.temperature_c     = 24.3f;
    t.sensors.humidity_pct      = 40.0f;
    t.sensors.soil_moisture_pct = 12.0f;
    t.sensors.water_level_ok    = true;
    t.sensors.is_daytime        = true;
    t.actuators.fan_on          = true;
    t.actuators.pump_on         = true;
    t.actuators.sensor_fault    = false;

    char buf[192];
    int n = telemetry_to_human(&t, buf, sizeof(buf));
    assert(n > 0);
    assert(strstr(buf, "T:24C")     != NULL);
    assert(strstr(buf, "Air:40%")   != NULL);
    assert(strstr(buf, "Soil:12%")  != NULL);
    assert(strstr(buf, "Fan:ON")    != NULL);
    assert(strstr(buf, "Pump:ON")   != NULL);
    assert(strstr(buf, "Water:OK")  != NULL);
    assert(strstr(buf, "Day:YES")   != NULL);
    assert(strstr(buf, "Fault:NO")  != NULL);
}

static void test_low_and_night(void) {
    Telemetry_t t = {0};
    t.sensors.water_level_ok = false;
    t.sensors.is_daytime     = false;
    t.actuators.sensor_fault = true;

    char buf[192];
    assert(telemetry_to_human(&t, buf, sizeof(buf)) > 0);
    assert(strstr(buf, "Water:LOW") != NULL);
    assert(strstr(buf, "Day:NO")    != NULL);
    assert(strstr(buf, "Fault:YES") != NULL);
}

static void test_truncation_detected(void) {
    Telemetry_t t = {0};
    char buf[8];
    assert(telemetry_to_human(&t, buf, sizeof(buf)) == -1);
}

int main(void) {
    test_basic_format();
    test_low_and_night();
    test_truncation_detected();
    printf("test_telemetry_human: OK\n");
    return 0;
}