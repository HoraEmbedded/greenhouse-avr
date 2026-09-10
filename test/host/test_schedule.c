/* Host-side unit tests for is_daytime() in schedule.c. */

#include <stdio.h>
#include "../../src/schedule.h"

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
    printf("--- is_daytime() [window: %d-%d] ---\n",
           DAYTIME_START_HOUR, DAYTIME_END_HOUR);

    CHECK("midnight is night", is_daytime(0) == 0);
    CHECK("just before the window opens is still night",
          is_daytime(DAYTIME_START_HOUR - 1) == 0);
    CHECK("the exact start hour is daytime (inclusive lower bound)",
          is_daytime(DAYTIME_START_HOUR) == 1);
    CHECK("noon is daytime", is_daytime(12) == 1);
    CHECK("the hour just before the window closes is still daytime",
          is_daytime(DAYTIME_END_HOUR - 1) == 1);
    CHECK("the exact end hour is night (exclusive upper bound)",
          is_daytime(DAYTIME_END_HOUR) == 0);
    CHECK("late evening is night", is_daytime(23) == 0);

    printf("\n");
    if (failures == 0) {
        printf("=== all tests passed ===\n");
        return 0;
    }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
