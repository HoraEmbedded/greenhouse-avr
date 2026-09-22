#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>

#define RING_BUFFER_SIZE 32
#define RING_BUFFER_MASK (RING_BUFFER_SIZE - 1)

typedef struct {
    uint8_t buffer[RING_BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t overflow_count;
} RingBuffer;

void ring_buffer_init(RingBuffer *rb);
uint8_t ring_buffer_push(RingBuffer *rb, uint8_t byte);  /* 0 if full */
uint8_t ring_buffer_available(const RingBuffer *rb);
uint8_t ring_buffer_pop(RingBuffer *rb);

#endif
