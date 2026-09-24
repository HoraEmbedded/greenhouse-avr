#include "publisher.h"
#include <Arduino.h>
#include "telemetry_2.h"

#define PUBLISH_BUF_SIZE 192

bool publisher_init(void)     { return true; }
bool publisher_is_ready(void) { return true; }

bool publisher_publish(const Telemetry_t *t)
{
    char buf[PUBLISH_BUF_SIZE];
    if (telemetry_to_human(t, buf, sizeof(buf)) < 0) return false;
    Serial.println(buf);
    return true;
}