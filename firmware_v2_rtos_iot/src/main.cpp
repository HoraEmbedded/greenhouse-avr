/**
 * @file main.cpp
 * @brief Point d'entrée du firmware V2 - Serre IoT (ESP32 + FreeRTOS)
 * @author HoraEmbedded
 * @date 2026
 *
 * Architecture :
 * - Task_Sensors  (Priorité 3, Cœur 0) : Lecture I2C des capteurs
 * - Task_Logic    (Priorité 2, Cœur 0) : Régulation (hystérésis, gardes)
 * - Task_Network  (Priorité 1, Cœur 1) : Wi-Fi, MQTT/TLS, publication JSON
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "config.h"
#include "task_sensors.h"
#include "task_logic.h"

QueueHandle_t     xSensorQueue   = NULL;
QueueHandle_t     xActuatorQueue = NULL;
SemaphoreHandle_t xI2CMutex      = NULL;

void setup()
{
    Serial.begin(115200);
    Serial.println("[V2] boot");

    xSensorQueue   = xQueueCreate(10, sizeof(SensorData_t));
    xActuatorQueue = xQueueCreate(1,  sizeof(ActuatorState_t));
    xI2CMutex      = xSemaphoreCreateMutex();

    if (!xSensorQueue || !xActuatorQueue || !xI2CMutex) {
        Serial.println("[V2] fatal: rtos alloc failed");
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    task_sensors_start(xSensorQueue, xI2CMutex);
    task_logic_start(xSensorQueue, xActuatorQueue);

    Serial.println("[V2] scheduler running");
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}