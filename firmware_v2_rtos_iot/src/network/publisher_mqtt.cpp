#include "publisher.h"
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"
#include "wifi_manager.h"
#include "telemetry_json.h"

#define PUBLISH_BUF_SIZE 256

static WiFiClientSecure s_tls;
static PubSubClient    s_mqtt(s_tls);

bool publisher_init(void)
{
    wifi_manager_init();
    s_tls.setCACert(ROOT_CA);
    s_mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    s_mqtt.setBufferSize(PUBLISH_BUF_SIZE);
    return true;
}

static bool ensure_mqtt_connected(void)
{
    if (s_mqtt.connected()) return true;
    if (!wifi_manager_is_connected()) return false;

    Serial.printf("[MQTT] connecting to %s:%d\n", MQTT_BROKER, MQTT_PORT);
    bool ok = s_mqtt.connect(
        "greenhouse-node01",
        MQTT_USERNAME,
        MQTT_PASSWORD
    );
    if (ok) {
        Serial.println("[MQTT] connected");
    } else {
        Serial.printf("[MQTT] failed, state=%d\n", s_mqtt.state());
    }
    return ok;
}

bool publisher_is_ready(void)
{
    return wifi_manager_is_connected();
}

bool publisher_publish(const Telemetry_t *t)
{
    if (!ensure_mqtt_connected()) return false;

    char buf[PUBLISH_BUF_SIZE];
    if (telemetry_to_json(t, buf, sizeof(buf)) < 0) return false;

    bool ok = s_mqtt.publish(MQTT_TOPIC, buf);
    if (!ok) {
        Serial.println("[MQTT] publish failed");
    }
    return ok;
}