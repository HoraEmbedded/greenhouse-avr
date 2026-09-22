#include <Arduino.h>
#include "task_display.h"
#include "config.h"
#include "lcd.h"

static QueueHandle_t s_queue;

static void task_display_run(void *pvParameters)
{
    (void)pvParameters;

    lcd_init();
    lcd_show_ready();
    vTaskDelay(pdMS_TO_TICKS(1500));

    for (;;) {
        Telemetry_t t;
        if (xQueuePeek(s_queue, &t, pdMS_TO_TICKS(100)) == pdPASS) {
            if (t.actuators.sensor_fault) {
                lcd_show_sensor_error(t.actuators.dht_failures);
            } else {
                lcd_show_status(
                    t.sensors.temperature_c,
                    t.sensors.soil_moisture_pct,
                    t.actuators.fan_on,
                    t.actuators.pump_on,
                    t.sensors.water_level_ok);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_REFRESH_PERIOD_MS));
    }
}

void task_display_start(QueueHandle_t telemetry_queue, SemaphoreHandle_t i2c_mutex)
{
    (void)i2c_mutex; // mutex already shared via i2c_bus
    s_queue = telemetry_queue;

    xTaskCreatePinnedToCore(
        task_display_run,
        "Task_Display",
        TASK_DISPLAY_STACK,
        NULL,
        TASK_DISPLAY_PRIORITY,
        NULL,
        0
    );
}