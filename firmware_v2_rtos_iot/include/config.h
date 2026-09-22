#ifndef CONFIG_H
#define CONFIG_H

// --- Configuration des tâches ---
#define TASK_SENSORS_PRIORITY   3
#define TASK_LOGIC_PRIORITY     2
#define TASK_NETWORK_PRIORITY   1

#define TASK_SENSORS_STACK      4096
#define TASK_LOGIC_STACK        4096
#define TASK_NETWORK_STACK      8192

// --- Fréquences ---
#define SENSOR_SAMPLE_FREQ_HZ   20
#define SENSOR_SAMPLE_PERIOD_MS (1000 / SENSOR_SAMPLE_FREQ_HZ)

// --- Seuils d'hystérésis ---
#define FAN_ON_TEMP_C           26.0f
#define FAN_OFF_TEMP_C          24.0f
#define PUMP_ON_SOIL_PCT        30.0f
#define PUMP_OFF_SOIL_PCT       60.0f

// --- Structure de données capteurs ---
typedef struct {
    float temperature_c;
    float humidity_pct;
    float soil_moisture_pct;
    bool  water_level_ok;
    bool  is_daytime;
    uint32_t timestamp_ms;
} SensorData_t;

#endif // CONFIG_H