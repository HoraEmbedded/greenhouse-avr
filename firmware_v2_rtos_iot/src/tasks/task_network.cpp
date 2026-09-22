#include "task_network.h"
#include "config.h"
#include "publisher.h"

static QueueHandle_t s_queue;

static void task_network_run(void *pvParameters)
{
    (void)pvParameters;

    publisher_init();

    for (;;) {
        Telemetry_t t;
        if (xQueuePeek(s_queue, &t, portMAX_DELAY) == pdPASS) {
            if (publisher_is_ready()) {
                publisher_publish(&t);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(NETWORK_PUBLISH_PERIOD_MS));
    }
}

void task_network_start(QueueHandle_t telemetry_queue)
{
    s_queue = telemetry_queue;

    xTaskCreatePinnedToCore(
        task_network_run,
        "Task_Network",
        TASK_NETWORK_STACK,
        NULL,
        TASK_NETWORK_PRIORITY,
        NULL,
        1
    );
}