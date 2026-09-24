#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "config.h"
#include "sensors.h"
#include "task_sensors.h"
#include "task_logic.h"
#include "task_network.h"
#include "task_display.h"

static QueueHandle_t     s_sensor_queue    = NULL;
static QueueHandle_t     s_telemetry_queue = NULL;
static SemaphoreHandle_t s_i2c_mutex       = NULL;

void setup()
{
    Serial.begin(115200);
    Serial.println("[V2] boot");

    s_sensor_queue    = xQueueCreate(10, sizeof(SensorData_t));
    s_telemetry_queue = xQueueCreate(1,  sizeof(Telemetry_t));
    s_i2c_mutex       = xSemaphoreCreateMutex();

    if (!s_sensor_queue || !s_telemetry_queue || !s_i2c_mutex) {
        Serial.println("[V2] fatal: rtos alloc failed");
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    actuators_init();

    task_sensors_start(s_sensor_queue, s_i2c_mutex);
    task_logic_start(s_sensor_queue, s_telemetry_queue);
    task_network_start(s_telemetry_queue);
    task_display_start(s_telemetry_queue, s_i2c_mutex);
    Serial.println("[V2] scheduler running");
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}