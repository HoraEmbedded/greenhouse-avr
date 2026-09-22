#include "ring_buffer.h"

void ring_buffer_init(RingBuffer *rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->overflow_count = 0;
}

uint8_t ring_buffer_push(RingBuffer *rb, uint8_t byte) {
    uint8_t next_head = (uint8_t)((rb->head + 1) & RING_BUFFER_MASK);
    if (next_head == rb->tail) {
        if (rb->overflow_count < 255) rb->overflow_count++;
        return 0;
    }
    rb->buffer[rb->head] = byte;
    rb->head = next_head;
    return 1;
}

uint8_t ring_buffer_available(const RingBuffer *rb) {
    return rb->head != rb->tail;
}

uint8_t ring_buffer_pop(RingBuffer *rb) {
    uint8_t byte = rb->buffer[rb->tail];
    rb->tail = (uint8_t)((rb->tail + 1) & RING_BUFFER_MASK);
    return byte;
}
