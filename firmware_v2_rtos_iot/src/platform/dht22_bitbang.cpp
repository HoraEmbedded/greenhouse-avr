#include "dht22.h"
#include <Arduino.h>
#include "board.h"
#include "dht22_decode.h"

#define DHT_TIMEOUT_LOOPS 10000
#define DHT_SAMPLE_US     40

static bool wait_pin(uint8_t level, uint16_t timeout_loops)
{
    while (digitalRead(PIN_DHT22) != level) {
        if (--timeout_loops == 0) return false;
    }
    return true;
}

void dht22_init(void)
{
    pinMode(PIN_DHT22, INPUT_PULLUP);
}

bool dht22_read(float *temperature_c, float *humidity_pct)
{
    uint8_t data[5] = {0};

    pinMode(PIN_DHT22, OUTPUT);
    digitalWrite(PIN_DHT22, LOW);
    delay(20);
    digitalWrite(PIN_DHT22, HIGH);
    delayMicroseconds(30);
    pinMode(PIN_DHT22, INPUT_PULLUP);

    if (!wait_pin(LOW,  DHT_TIMEOUT_LOOPS)) return false;
    if (!wait_pin(HIGH, DHT_TIMEOUT_LOOPS)) return false;
    if (!wait_pin(LOW,  DHT_TIMEOUT_LOOPS)) return false;

    for (uint8_t i = 0; i < 5; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            if (!wait_pin(HIGH, DHT_TIMEOUT_LOOPS)) return false;
            delayMicroseconds(DHT_SAMPLE_US);
            if (digitalRead(PIN_DHT22) == HIGH) {
                data[i] |= (1 << (7 - j));
                if (!wait_pin(LOW, DHT_TIMEOUT_LOOPS)) return false;
            }
        }
    }

    if (!dht22_checksum_valid(data)) return false;

    Dht22Reading r = dht22_decode(data);
    *temperature_c = r.temperature_decidegC / 10.0f;
    *humidity_pct  = r.air_humidity_decipercent / 10.0f;
    return true;
}