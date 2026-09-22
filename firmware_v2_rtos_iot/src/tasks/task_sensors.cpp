#include "task_sensors.h"
#include "config.h"
#include "sensors.h"

static QueueHandle_t s_queue;

static void task_sensors_run(void *pvParameters)
{
    (void)pvParameters;

    uint8_t dht_failures = 0;
    const TickType_t period = pdMS_TO_TICKS(SENSOR_SAMPLE_PERIOD_MS);
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        SensorData_t data;
        sensors_read_all(&data, &dht_failures);

        if (xQueueSend(s_queue, &data, 0) != pdPASS) {
            SensorData_t discard;
            xQueueReceive(s_queue, &discard, 0);
            xQueueSend(s_queue, &data, 0);
        }

        vTaskDelayUntil(&last_wake, period);
    }
}

void task_sensors_start(QueueHandle_t sensor_queue, SemaphoreHandle_t i2c_mutex)
{
    s_queue = sensor_queue;
    sensors_init(i2c_mutex);

    xTaskCreatePinnedToCore(
        task_sensors_run,
        "Task_Sensors",
        TASK_SENSORS_STACK,
        NULL,
        TASK_SENSORS_PRIORITY,
        NULL,
        0
    );
}