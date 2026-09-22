#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define TASK_SENSORS_PRIORITY   3
#define TASK_LOGIC_PRIORITY     2
#define TASK_NETWORK_PRIORITY   1

#define TASK_SENSORS_STACK      4096
#define TASK_LOGIC_STACK        4096
#define TASK_NETWORK_STACK      8192

#define SENSOR_SAMPLE_FREQ_HZ   20
#define SENSOR_SAMPLE_PERIOD_MS (1000 / SENSOR_SAMPLE_FREQ_HZ)

typedef struct {
    float    temperature_c;
    float    humidity_pct;
    float    soil_moisture_pct;
    bool     water_level_ok;
    bool     is_daytime;
    uint32_t timestamp_ms;
} SensorData_t;

typedef struct {
    bool     fan_on;
    bool     pump_on;
    bool     sensor_fault;
    uint8_t  dht_failures;
    uint32_t timestamp_ms;
} ActuatorState_t;

#endif