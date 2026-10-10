#include "ring_buffer.h"

bool ring_buffer_init(ring_buffer_t *rb, uint8_t *storage, size_t capacity)
{
    if (rb == NULL || storage == NULL || capacity == 0) {
        return false;
    }

    rb->storage = storage;
    rb->capacity = capacity;
    ring_buffer_reset(rb);
    return true;
}

void ring_buffer_reset(ring_buffer_t *rb)
{
    if (rb == NULL) {
        return;
    }

    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool ring_buffer_is_empty(const ring_buffer_t *rb)
{
    return rb == NULL || rb->count == 0;
}

bool ring_buffer_is_full(const ring_buffer_t *rb)
{
    return rb != NULL && rb->count == rb->capacity;
}

size_t ring_buffer_size(const ring_buffer_t *rb)
{
    return rb == NULL ? 0 : rb->count;
}

size_t ring_buffer_capacity(const ring_buffer_t *rb)
{
    return rb == NULL ? 0 : rb->capacity;
}

bool ring_buffer_push(ring_buffer_t *rb, uint8_t value)
{
    if (rb == NULL || rb->storage == NULL || ring_buffer_is_full(rb)) {
        return false;
    }

    rb->storage[rb->head] = value;
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count++;
    return true;
}

bool ring_buffer_pop(ring_buffer_t *rb, uint8_t *out_value)
{
    if (rb == NULL || rb->storage == NULL || out_value == NULL || ring_buffer_is_empty(rb)) {
        return false;
    }

    *out_value = rb->storage[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count--;
    return true;
}

bool ring_buffer_peek(const ring_buffer_t *rb, uint8_t *out_value)
{
    if (rb == NULL || rb->storage == NULL || out_value == NULL || ring_buffer_is_empty(rb)) {
        return false;
    }

    *out_value = rb->storage[rb->tail];
    return true;
}

