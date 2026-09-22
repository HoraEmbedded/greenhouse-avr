#ifndef DHT22_DECODE_H
#define DHT22_DECODE_H

#include <stdint.h>

typedef struct {
    int16_t  temperature_decidegC;
    uint16_t air_humidity_decipercent;
} Dht22Reading;

/* Checksum: byte 4 vs sum of bytes 0-3, mod 256. */
uint8_t dht22_checksum_valid(const uint8_t data[5]);

/* Bit 7 of byte 2 = sign; bits 2-3 = magnitude. */
Dht22Reading dht22_decode(const uint8_t data[5]);

#endif
