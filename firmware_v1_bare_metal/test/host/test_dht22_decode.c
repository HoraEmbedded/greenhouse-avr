#include <stdio.h>
#include <stdint.h>
#include "../../src/dht22_decode.h"

static int failures = 0;
#define CHECK(desc, cond) do { if (cond) printf("  [OK] %s\n", desc); else { printf("  [FAIL] %s\n", desc); failures++; } } while (0)

static void make_frame(uint8_t f[5], uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
    f[0]=b0; f[1]=b1; f[2]=b2; f[3]=b3; f[4]=(uint8_t)(b0+b1+b2+b3);
}

int main(void) {
    uint8_t frame[5];

    printf("--- dht22_checksum_valid() ---\n");
    make_frame(frame, 0x01, 0x90, 0x00, 0xA0);
    CHECK("valid checksum accepted", dht22_checksum_valid(frame) == 1);
    frame[4] ^= 0xFF;
    CHECK("corrupted checksum rejected", dht22_checksum_valid(frame) == 0);
    make_frame(frame, 0xFF, 0xFF, 0xFF, 0xFF);
    CHECK("checksum wraps mod 256", dht22_checksum_valid(frame) == 1);

    printf("\n--- dht22_decode() ---\n");
    make_frame(frame, 0x01, 0x90, 0x00, 0xA0);
    Dht22Reading r = dht22_decode(frame);
    CHECK("humidity bytes combine", r.air_humidity_decipercent == 400);
    CHECK("positive temp decodes", r.temperature_decidegC == 160);

    make_frame(frame, 0x02, 0x14, 0x80, 0x32);
    r = dht22_decode(frame);
    CHECK("humidity ok alongside negative temp", r.air_humidity_decipercent == 532);
    CHECK("sign bit -> negative temp", r.temperature_decidegC == -50);

    make_frame(frame, 0x00, 0x00, 0x80, 0x00);
    r = dht22_decode(frame);
    CHECK("negative zero -> 0", r.temperature_decidegC == 0);

    make_frame(frame, 0x03, 0x84, 0x01, 0x0E);
    r = dht22_decode(frame);
    CHECK("humidity high byte not dropped", r.air_humidity_decipercent == 900);

    printf("\n");
    if (failures == 0) { printf("=== all tests passed ===\n"); return 0; }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
