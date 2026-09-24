#include <stdio.h>
#include "../../src/rtc_decode.h"

static int failures = 0;
#define CHECK(desc, cond) do { if (cond) printf("  [OK] %s\n", desc); else { printf("  [FAIL] %s\n", desc); failures++; } } while (0)

int main(void) {
    printf("--- bcd_to_decimal() ---\n");
    CHECK("0x00 -> 0", bcd_to_decimal(0x00) == 0);
    CHECK("0x09 -> 9", bcd_to_decimal(0x09) == 9);
    CHECK("0x10 -> 10", bcd_to_decimal(0x10) == 10);
    CHECK("0x23 -> 23", bcd_to_decimal(0x23) == 23);
    CHECK("0x59 -> 59", bcd_to_decimal(0x59) == 59);
    CHECK("0x23 != 35 (not raw binary)", bcd_to_decimal(0x23) != 35);
    printf("\n");
    if (failures == 0) { printf("=== all tests passed ===\n"); return 0; }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
