#include "dht22_decode.h"

uint8_t dht22_checksum_valid(const uint8_t data[5]) {
    uint8_t sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    return sum == data[4];
}

Dht22Reading dht22_decode(const uint8_t data[5]) {
    Dht22Reading r;
    r.air_humidity_decipercent = (uint16_t)((data[0] << 8) | data[1]);

    int16_t magnitude = (int16_t)(((data[2] & 0x7F) << 8) | data[3]);
    r.temperature_decidegC = (data[2] & 0x80) ? (int16_t)(-magnitude) : magnitude;

    return r;
}
