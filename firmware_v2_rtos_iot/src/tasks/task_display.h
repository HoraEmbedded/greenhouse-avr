#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

void task_display_start(QueueHandle_t telemetry_queue, SemaphoreHandle_t i2c_mutex);

#endif