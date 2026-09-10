#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>

/* A small fixed-size ring buffer, no dynamic allocation, no I/O -- the
 * same principle as everywhere else in this project: separate the
 * DECISION (does this byte fit, what comes out next) from the hardware
 * that produces or consumes the bytes. main.c's USART0_RX_vect ISR is a
 * thin wrapper around ring_buffer_push(); uart_available()/
 * uart_read_char() are thin wrappers around the read side. Neither the
 * ISR nor the wrappers have any logic of their own left to get wrong.
 */

#define RING_BUFFER_SIZE 32  // power of 2: wraparound is a cheap bitmask
#define RING_BUFFER_MASK (RING_BUFFER_SIZE - 1)

typedef struct {
    uint8_t buffer[RING_BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t overflow_count;
} RingBuffer;

void ring_buffer_init(RingBuffer *rb);

/* Returns 1 if byte was stored, 0 if the buffer was full (byte dropped,
 * overflow_count incremented). Dropping the NEW byte rather than
 * overwriting the oldest unread one is deliberate: it fails as "the tail
 * of a long line is missing", not as silent data corruption. */
uint8_t ring_buffer_push(RingBuffer *rb, uint8_t byte);

uint8_t ring_buffer_available(const RingBuffer *rb);

/* Undefined result if called when ring_buffer_available() is 0 -- same
 * contract as main.c's uart_read_char(), which only ever calls this
 * after checking uart_available(). */
uint8_t ring_buffer_pop(RingBuffer *rb);

#endif
