/* Host-side unit tests for ring_buffer.c. No AVR, no ISR, no UART --
 * this is the same push/pop logic the interrupt handler and main()
 * actually call, just driven directly. */

#include <stdio.h>
#include "../../src/ring_buffer.h"

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

static void test_basic_fifo(void) {
    printf("--- FIFO order and availability ---\n");

    RingBuffer rb;
    ring_buffer_init(&rb);

    CHECK("empty buffer reports nothing available",
          ring_buffer_available(&rb) == 0);

    ring_buffer_push(&rb, 'A');
    ring_buffer_push(&rb, 'B');
    ring_buffer_push(&rb, 'C');

    CHECK("bytes come out in the order they went in (FIFO, not LIFO)",
          ring_buffer_pop(&rb) == 'A' &&
          ring_buffer_pop(&rb) == 'B' &&
          ring_buffer_pop(&rb) == 'C');

    CHECK("buffer is empty again after popping everything pushed",
          ring_buffer_available(&rb) == 0);
}

static void test_wraparound(void) {
    /* Pushes and pops enough times to wrap the head/tail indices past
     * RING_BUFFER_SIZE several times over -- the exact scenario a long-
     * running greenhouse (weeks between resets) will exercise for real,
     * not just once at start-of-day. */
    printf("--- Index wraparound over many cycles ---\n");

    RingBuffer rb;
    ring_buffer_init(&rb);
    int mismatches = 0;

    for (int cycle = 0; cycle < 10 * RING_BUFFER_SIZE; cycle++) {
        uint8_t sent = (uint8_t)(cycle & 0xFF);
        ring_buffer_push(&rb, sent);
        uint8_t received = ring_buffer_pop(&rb);
        if (received != sent) mismatches++;
    }

    CHECK("1 push + 1 pop repeated past several full wraps: every byte "
          "comes back correct",
          mismatches == 0);
    CHECK("no overflow recorded (buffer never held more than one byte "
          "at a time in this pattern)",
          rb.overflow_count == 0);
}

static void test_overflow(void) {
    printf("--- Overflow: buffer full, new bytes dropped ---\n");

    RingBuffer rb;
    ring_buffer_init(&rb);

    /* The buffer can hold RING_BUFFER_SIZE - 1 bytes, not
     * RING_BUFFER_SIZE: the last slot is deliberately left empty so
     * head == tail can only ever mean "empty", never ambiguous with
     * "completely full" -- the standard ring-buffer trick to keep
     * available()/full checks a single comparison instead of a
     * separate counter. */
    uint8_t accepted = 0;
    for (int i = 0; i < RING_BUFFER_SIZE + 5; i++) {
        if (ring_buffer_push(&rb, (uint8_t)i)) accepted++;
    }

    CHECK("exactly capacity-minus-one bytes accepted before the buffer "
          "reports full",
          accepted == RING_BUFFER_SIZE - 1);

    /* Pushed RING_BUFFER_SIZE + 5 bytes total; only RING_BUFFER_SIZE - 1
     * fit (see the comment above), so the remainder is
     * (RING_BUFFER_SIZE + 5) - (RING_BUFFER_SIZE - 1) = 6, not 5 -- easy
     * off-by-one to get wrong by eye, which is exactly why this is
     * computed here instead of hardcoded. */
    CHECK("the excess pushes were counted as overflow, not silently lost",
          rb.overflow_count == (RING_BUFFER_SIZE + 5) - (RING_BUFFER_SIZE - 1));

    CHECK("the bytes that WERE accepted are still there, in order, "
          "after the overflow (dropping new bytes didn't corrupt old ones)",
          ring_buffer_pop(&rb) == 0 &&
          ring_buffer_pop(&rb) == 1);
}

static void test_drain_then_refill(void) {
    /* After a full drain, the buffer must behave exactly like a fresh
     * one -- not left in some half-reset state from wherever head/tail
     * happened to land. */
    printf("--- Buffer works normally again after a full drain ---\n");

    RingBuffer rb;
    ring_buffer_init(&rb);

    for (int i = 0; i < RING_BUFFER_SIZE - 1; i++) ring_buffer_push(&rb, (uint8_t)i);
    while (ring_buffer_available(&rb)) ring_buffer_pop(&rb);

    ring_buffer_push(&rb, 42);
    CHECK("a fresh push after a full drain is received correctly",
          ring_buffer_available(&rb) == 1 && ring_buffer_pop(&rb) == 42);
}

static void test_overflow_counter_saturates(void) {
    /* overflow_count is a uint8_t: without the < 255 guard on line 12 of
     * ring_buffer.c, the 256th overflow would silently wrap to 0,
     * making a long-running fault look like it had just started. */
    printf("--- Overflow counter saturates at 255, doesn't wrap to 0 ---\n");

    RingBuffer rb;
    ring_buffer_init(&rb);

    for (int i = 0; i < RING_BUFFER_SIZE - 1; i++) ring_buffer_push(&rb, 0);
    for (int i = 0; i < 300; i++) ring_buffer_push(&rb, 0);  // all rejected

    CHECK("overflow_count caps at 255 instead of wrapping past it",
          rb.overflow_count == 255);
}

int main(void) {
    test_basic_fifo();
    test_wraparound();
    test_overflow();
    test_overflow_counter_saturates();
    test_drain_then_refill();

    printf("\n");
    if (failures == 0) {
        printf("=== all tests passed ===\n");
        return 0;
    }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
