#include "task_logic.h"
#include "config.h"
#include "sensors.h"

#include "hysteresis.h"
#include "thresholds.h"
#include "fault_handling.h"
#include "water_level.h"
#include "schedule.h"

static QueueHandle_t s_sensor_queue;
static QueueHandle_t s_telemetry_queue;

static void task_logic_run(void *pvParameters)
{
    (void)pvParameters;

    GreenhouseThresholds config = {
        .fan_on_decidegC  = 260,
        .fan_off_decidegC = 240,
        .pump_on_percent  = 30,
        .pump_off_percent = 60,
    };

    bool     fan_state   = false;
    bool     pump_state  = false;
    uint8_t  dht_failures = 0;
    uint32_t sequence    = 0;

    SensorData_t    in;
    ActuatorState_t act;
    Telemetry_t     tele;

    for (;;) {
        if (xQueueReceive(s_sensor_queue, &in, portMAX_DELAY) != pdPASS) {
            continue;
        }

        bool sensor_fault = (in.temperature_c <= -100.0f);

        if (!sensor_fault) {
            dht_failures = 0;
            int16_t temp_dc = (int16_t)(in.temperature_c * 10.0f + 0.5f);
            fan_state = fan_hysteresis(fan_state, temp_dc, &config);
        } else {
            if (dht_failures < 255) dht_failures++;
            fan_state = degraded_fan_state(dht_failures, fan_state);
        }

        int8_t soil_pct = (int8_t)(in.soil_moisture_pct + 0.5f);
        pump_state = pump_hysteresis(pump_state, soil_pct, &config);
        pump_state = pump_output_state(pump_state, in.water_level_ok);
        if (!in.is_daytime) pump_state = false;

        act.fan_on       = fan_state;
        act.pump_on      = pump_state;
        act.sensor_fault = sensor_fault;
        act.dht_failures = dht_failures;
        act.timestamp_ms = in.timestamp_ms;

        actuators_apply(&act);

        tele.sensors  = in;
        tele.actuators = act;
        tele.sequence  = ++sequence;
        xQueueOverwrite(s_telemetry_queue, &tele);
    }
}

void task_logic_start(QueueHandle_t sensor_queue, QueueHandle_t telemetry_queue)
{
    s_sensor_queue    = sensor_queue;
    s_telemetry_queue = telemetry_queue;

    xTaskCreatePinnedToCore(
        task_logic_run,
        "Task_Logic",
        TASK_LOGIC_STACK,
        NULL,
        TASK_LOGIC_PRIORITY,
        NULL,
        0
    );
}