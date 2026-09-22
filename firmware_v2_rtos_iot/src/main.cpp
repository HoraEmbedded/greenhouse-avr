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

// --- Déclarations des handles FreeRTOS ---
QueueHandle_t xSensorQueue = NULL;
SemaphoreHandle_t xI2CMutex = NULL;

// --- Prototypes des tâches ---
void vTaskSensors(void *pvParameters);
void vTaskLogic(void *pvParameters);
void vTaskNetwork(void *pvParameters);

void setup() {
    Serial.begin(115200);
    Serial.println("[V2] Démarrage de la Serre IoT...");

    // Queue pour les données capteurs
    xSensorQueue = xQueueCreate(10, sizeof(SensorData_t));
    if (xSensorQueue == NULL) {
        Serial.println("[ERREUR] Impossible de créer la Queue.");
        while (1);
    }

    // protéger le bus I2C
    xI2CMutex = xSemaphoreCreateMutex();
    if (xI2CMutex == NULL) {
        Serial.println("[ERREUR] Impossible de créer le Mutex I2C.");
        while (1);
    }

    // les tâches
    xTaskCreatePinnedToCore(vTaskSensors, "Task_Sensors", 4096, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(vTaskLogic,   "Task_Logic",   4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(vTaskNetwork, "Task_Network", 8192, NULL, 1, NULL, 1);

    Serial.println("[V2] Tâches créées. Ordonnanceur FreeRTOS en cours...");
}

void loop() {
    
}


void vTaskSensors(void *pvParameters) {
    (void)pvParameters;
    const TickType_t xFrequency = pdMS_TO_TICKS(50); // 20 Hz
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void vTaskLogic(void *pvParameters) {
    (void)pvParameters;
    SensorData_t data;
    for (;;) {
        if (xQueueReceive(xSensorQueue, &data, portMAX_DELAY) == pdPASS) {
            }
    }
}

void vTaskNetwork(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000)); // Temporisation 
    }
}