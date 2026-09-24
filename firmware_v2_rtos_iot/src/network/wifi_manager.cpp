#include "wifi_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"

#define WIFI_RETRY_BASE_MS 2000
#define WIFI_RETRY_MAX_MS  30000

static uint32_t s_backoff_ms = WIFI_RETRY_BASE_MS;
static uint32_t s_last_attempt_ms = 0;

void wifi_manager_init(void)
{
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("[WIFI] connecting to %s\n", WIFI_SSID);
}

bool wifi_manager_is_connected(void)
{
    return WiFi.status() == WL_CONNECTED;
}

bool wifi_manager_ensure_connected(void)
{
    if (wifi_manager_is_connected()) {
        s_backoff_ms = WIFI_RETRY_BASE_MS;
        return true;
    }

    uint32_t now = millis();
    if ((uint32_t)(now - s_last_attempt_ms) < s_backoff_ms) {
        return false;
    }
    s_last_attempt_ms = now;

    Serial.printf("[WIFI] retry (backoff=%lu ms)\n", (unsigned long)s_backoff_ms);
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    s_backoff_ms *= 2;
    if (s_backoff_ms > WIFI_RETRY_MAX_MS) {
        s_backoff_ms = WIFI_RETRY_MAX_MS;
    }
    return false;
}