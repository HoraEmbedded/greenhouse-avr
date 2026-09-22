#ifndef TASK_NETWORK_H
#define TASK_NETWORK_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

void task_network_start(QueueHandle_t telemetry_queue);

#endif