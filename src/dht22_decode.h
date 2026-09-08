#ifndef DHT22_DECODE_H
#define DHT22_DECODE_H

#include <stdint.h>

/* Pure decoding of an already-received DHT22 frame -- no GPIO, no timing,
 * no bus. dht_read() in main.c still owns the 1-Wire bit-banging (that
 * part genuinely cannot be tested without the bus, real or simulated),
 * but what the 5 bytes MEAN once they're in hand is ordinary arithmetic,
 * and ordinary arithmetic is testable on the host
 * (test/host/test_dht22_decode.c). */

typedef struct {
    int16_t  temperature_decidegC;      /* tenths of a degree C, signed */
    uint16_t air_humidity_decipercent;  /* tenths of a percent */
} Dht22Reading;

/* Validates the frame's checksum (byte 4 against the sum of bytes 0-3,
 * wrapped modulo 256). Call this BEFORE dht22_decode() -- decode does not
 * re-check it, matching how dht_read() already uses the two together. */
uint8_t dht22_checksum_valid(const uint8_t data[5]);

/* Decodes a frame into signed temperature and humidity. Bit 7 of byte 2
 * is the DHT22's sign bit for temperature; the remaining 15 bits across
 * bytes 2-3 are the magnitude. */
Dht22Reading dht22_decode(const uint8_t data[5]);

#endif
