/* Host-side unit tests for dht22_checksum_valid() and dht22_decode() in
 * dht22_decode.c. */

#include <stdio.h>
#include <stdint.h>
#include "../../src/dht22_decode.h"

static int failures = 0;

#define CHECK(desc, cond)                                              \
    do {                                                               \
        if (cond) {                                                    \
            printf("  [OK] %s\n", desc);                               \
        } else {                                                       \
            printf("  [FAIL] %s\n", desc);                             \
            failures++;                                                \
        }                                                               \
    } while (0)

/* Fills a frame's first 4 bytes and computes byte 4 the same way the
 * DHT22 itself does -- but independently of dht22_checksum_valid(), so
 * this is a genuine cross-check, not the same arithmetic written twice
 * and trusted to agree with itself. */
static void make_frame(uint8_t f[5], uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
    f[0] = b0; f[1] = b1; f[2] = b2; f[3] = b3;
    f[4] = (uint8_t)(b0 + b1 + b2 + b3);
}

int main(void) {
    uint8_t frame[5];

    printf("--- dht22_checksum_valid() ---\n");

    make_frame(frame, 0x01, 0x90, 0x00, 0xA0);
    CHECK("a correctly computed checksum validates",
          dht22_checksum_valid(frame) == 1);

    frame[4] ^= 0xFF;
    CHECK("a corrupted checksum is rejected",
          dht22_checksum_valid(frame) == 0);

    /* 0xFF+0xFF+0xFF+0xFF = 0x3FC, which does NOT fit in a byte: this
     * only passes if the checksum is computed modulo 256, matching what
     * the sensor itself does -- a naive 16-bit sum would reject a
     * perfectly valid frame here. */
    make_frame(frame, 0xFF, 0xFF, 0xFF, 0xFF);
    CHECK("checksum arithmetic wraps correctly at 0xFF sums (mod 256, "
          "not a 16-bit sum)",
          dht22_checksum_valid(frame) == 1);

    printf("\n--- dht22_decode() ---\n");

    make_frame(frame, 0x01, 0x90, 0x00, 0xA0); /* 40.0% humidity, +16.0C */
    Dht22Reading reading = dht22_decode(frame);
    CHECK("humidity high/low bytes combine correctly",
          reading.air_humidity_decipercent == 400);
    CHECK("a positive temperature decodes correctly",
          reading.temperature_decidegC == 160);

    make_frame(frame, 0x02, 0x14, 0x80, 0x32); /* 53.2% humidity, -5.0C */
    reading = dht22_decode(frame);
    CHECK("humidity decodes correctly alongside a negative temperature",
          reading.air_humidity_decipercent == 532);
    CHECK("the sign bit (0x80 in byte 2) produces a negative temperature",
          reading.temperature_decidegC == -50);

    make_frame(frame, 0x00, 0x00, 0x80, 0x00); /* sign bit set, magnitude 0 */
    reading = dht22_decode(frame);
    CHECK("a zero magnitude with the sign bit set still decodes as 0, "
          "not a large negative number",
          reading.temperature_decidegC == 0);

    make_frame(frame, 0x03, 0x84, 0x01, 0x0E); /* humidity byte 0 non-zero */
    reading = dht22_decode(frame);
    CHECK("byte 0 of humidity is not silently dropped",
          reading.air_humidity_decipercent == 900);

    printf("\n");
    if (failures == 0) {
        printf("=== all tests passed ===\n");
        return 0;
    }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
