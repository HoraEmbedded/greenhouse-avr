#include <stdio.h>
#include "../../src/ring_buffer.h"

static int failures = 0;
#define CHECK(desc, cond) do { if (cond) printf("  [OK] %s\n", desc); else { printf("  [FAIL] %s\n", desc); failures++; } } while (0)

static void test_basic_fifo(void) {
    printf("--- FIFO order ---\n");
    RingBuffer rb;
    ring_buffer_init(&rb);
    CHECK("empty reports unavailable", ring_buffer_available(&rb) == 0);
    ring_buffer_push(&rb, 'A'); ring_buffer_push(&rb, 'B'); ring_buffer_push(&rb, 'C');
    CHECK("FIFO order", ring_buffer_pop(&rb) == 'A' && ring_buffer_pop(&rb) == 'B' && ring_buffer_pop(&rb) == 'C');
    CHECK("empty after drain", ring_buffer_available(&rb) == 0);
}

static void test_wraparound(void) {
    printf("--- index wraparound ---\n");
    RingBuffer rb;
    ring_buffer_init(&rb);
    int mismatches = 0;
    for (int cycle = 0; cycle < 10 * RING_BUFFER_SIZE; cycle++) {
        uint8_t sent = (uint8_t)(cycle & 0xFF);
        ring_buffer_push(&rb, sent);
        if (ring_buffer_pop(&rb) != sent) mismatches++;
    }
    CHECK("survives many wraps", mismatches == 0);
    CHECK("no overflow in this pattern", rb.overflow_count == 0);
}

static void test_overflow(void) {
    printf("--- overflow ---\n");
    RingBuffer rb;
    ring_buffer_init(&rb);
    uint8_t accepted = 0;
    for (int i = 0; i < RING_BUFFER_SIZE + 5; i++) if (ring_buffer_push(&rb, (uint8_t)i)) accepted++;
    CHECK("capacity-1 accepted", accepted == RING_BUFFER_SIZE - 1);
    CHECK("excess counted as overflow", rb.overflow_count == (RING_BUFFER_SIZE + 5) - (RING_BUFFER_SIZE - 1));
    CHECK("accepted bytes still in order", ring_buffer_pop(&rb) == 0 && ring_buffer_pop(&rb) == 1);
}

static void test_overflow_saturates(void) {
    printf("--- overflow counter saturates ---\n");
    RingBuffer rb;
    ring_buffer_init(&rb);
    for (int i = 0; i < RING_BUFFER_SIZE - 1; i++) ring_buffer_push(&rb, 0);
    for (int i = 0; i < 300; i++) ring_buffer_push(&rb, 0);
    CHECK("caps at 255, no wrap", rb.overflow_count == 255);
}

static void test_drain_then_refill(void) {
    printf("--- refill after full drain ---\n");
    RingBuffer rb;
    ring_buffer_init(&rb);
    for (int i = 0; i < RING_BUFFER_SIZE - 1; i++) ring_buffer_push(&rb, (uint8_t)i);
    while (ring_buffer_available(&rb)) ring_buffer_pop(&rb);
    ring_buffer_push(&rb, 42);
    CHECK("works after full drain", ring_buffer_available(&rb) == 1 && ring_buffer_pop(&rb) == 42);
}

int main(void) {
    test_basic_fifo();
    test_wraparound();
    test_overflow();
    test_overflow_saturates();
    test_drain_then_refill();
    printf("\n");
    if (failures == 0) { printf("=== all tests passed ===\n"); return 0; }
    printf("=== %d test(s) FAILED ===\n", failures);
    return 1;
}
