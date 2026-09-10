/* Host-side unit tests for bcd_to_decimal() in rtc_decode.c. */

#include <stdio.h>
#include "../../src/rtc_decode.h"

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

int main(void) {
    printf("--- bcd_to_decimal() ---\n");

    CHECK("0x00 decodes to 0", bcd_to_decimal(0x00) == 0);
    CHECK("0x09 decodes to 9 (single digit, no tens nibble)",
          bcd_to_decimal(0x09) == 9);
    CHECK("0x10 decodes to 10 (tens nibble is 1, units is 0)",
          bcd_to_decimal(0x10) == 10);
    CHECK("0x23 decodes to 23 (a realistic hour value)",
          bcd_to_decimal(0x23) == 23);
    CHECK("0x59 decodes to 59 (a realistic minute/second value)",
          bcd_to_decimal(0x59) == 59);

    /* The bug this guards against: treating the byte as plain binary
     * instead of BCD. 0x23 as plain binary is 35, not 23 -- a wrong
     * implementation would pass every test above with small values by
     * coincidence up to 0x09, and only diverge once a tens digit
     * appears. This case exists specifically to catch that. */
    CHECK("0x23 is NOT decoded as the raw binary value 35",
          bcd_to_decimal(0x23) != 35);

    printf("\n");
    if (failures == 0) {
        printf("=== all tests passed ===\n");
        return 0;
    }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
