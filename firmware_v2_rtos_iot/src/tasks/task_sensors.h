#ifndef TASK_SENSORS_H
#define TASK_SENSORS_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

void task_sensors_start(QueueHandle_t sensor_queue, SemaphoreHandle_t i2c_mutex);

#endif