#ifndef TASK_LOGIC_H
#define TASK_LOGIC_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "config.h"

void task_logic_start(QueueHandle_t sensor_queue, QueueHandle_t actuator_queue);

#endif