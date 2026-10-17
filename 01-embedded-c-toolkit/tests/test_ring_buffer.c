#include "ring_buffer.h"

#include <assert.h>
#include <stdio.h>

static void test_init_rejects_bad_input(void)
{
    ring_buffer_t rb;
    uint8_t storage[4];

    assert(!ring_buffer_init(NULL, storage, sizeof(storage)));
    assert(!ring_buffer_init(&rb, NULL, sizeof(storage)));
    assert(!ring_buffer_init(&rb, storage, 0));
}

static void test_empty_buffer(void)
{
    ring_buffer_t rb;
    uint8_t storage[4];
    uint8_t value = 0xAA;

    assert(ring_buffer_init(&rb, storage, sizeof(storage)));
    assert(ring_buffer_is_empty(&rb));
    assert(!ring_buffer_is_full(&rb));
    assert(ring_buffer_size(&rb) == 0);
    assert(ring_buffer_capacity(&rb) == 4);
    assert(!ring_buffer_pop(&rb, &value));
    assert(value == 0xAA);
}

static void test_push_pop_fifo_order(void)
{
    ring_buffer_t rb;
    uint8_t storage[4];
    uint8_t value = 0;

    assert(ring_buffer_init(&rb, storage, sizeof(storage)));

    assert(ring_buffer_push(&rb, 10));
    assert(ring_buffer_push(&rb, 20));
    assert(ring_buffer_push(&rb, 30));

    assert(ring_buffer_size(&rb) == 3);

    assert(ring_buffer_pop(&rb, &value));
    assert(value == 10);
    assert(ring_buffer_pop(&rb, &value));
    assert(value == 20);
    assert(ring_buffer_pop(&rb, &value));
    assert(value == 30);
    assert(ring_buffer_is_empty(&rb));
}

static void test_full_buffer_rejects_push(void)
{
    ring_buffer_t rb;
    uint8_t storage[2];

    assert(ring_buffer_init(&rb, storage, sizeof(storage)));

    assert(ring_buffer_push(&rb, 1));
    assert(ring_buffer_push(&rb, 2));
    assert(ring_buffer_is_full(&rb));
    assert(!ring_buffer_push(&rb, 3));
    assert(ring_buffer_size(&rb) == 2);
}

static void test_wraparound(void)
{
    ring_buffer_t rb;
    uint8_t storage[3];
    uint8_t value = 0;

    assert(ring_buffer_init(&rb, storage, sizeof(storage)));

    assert(ring_buffer_push(&rb, 1));
    assert(ring_buffer_push(&rb, 2));
    assert(ring_buffer_push(&rb, 3));
    assert(ring_buffer_pop(&rb, &value));
    assert(value == 1);
    assert(ring_buffer_push(&rb, 4));

    assert(ring_buffer_pop(&rb, &value));
    assert(value == 2);
    assert(ring_buffer_pop(&rb, &value));
    assert(value == 3);
    assert(ring_buffer_pop(&rb, &value));
    assert(value == 4);
    assert(ring_buffer_is_empty(&rb));
}

static void test_peek_does_not_remove(void)
{
    ring_buffer_t rb;
    uint8_t storage[2];
    uint8_t value = 0;

    assert(ring_buffer_init(&rb, storage, sizeof(storage)));
    assert(ring_buffer_push(&rb, 42));

    assert(ring_buffer_peek(&rb, &value));
    assert(value == 42);
    assert(ring_buffer_size(&rb) == 1);

    assert(ring_buffer_pop(&rb, &value));
    assert(value == 42);
    assert(ring_buffer_is_empty(&rb));
}

static void test_capacity_one(void){
    ring_buffer_t rd;
    uint8_t storage[1];
    uint8_t value = 0;

    assert(ring_buffer_init(&rd, storage, sizeof(storage)));
    assert(ring_buffer_push(&rd, 42));
    assert(ring_buffer_is_full(&rd));
    assert(ring_buffer_size(&rd) == 1);

    assert(!ring_buffer_push(&rd, 99));
    assert(ring_buffer_size(&rd) == 1);

    assert(ring_buffer_peek(&rd, &value));
    assert(ring_buffer_is_full(&rd));

    assert(ring_buffer_pop(&rd, &value));
    assert(ring_buffer_is_empty(&rd));

}

int main(void)
{
    test_init_rejects_bad_input();
    test_empty_buffer();
    test_push_pop_fifo_order();
    test_full_buffer_rejects_push();
    test_wraparound();
    test_peek_does_not_remove();
    test_capacity_one();

    puts("ring_buffer tests passed");
    return 0;
}

