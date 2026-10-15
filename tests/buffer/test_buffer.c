#include "unity.h"
#include "buffers.h"

void setUp(void)
{
}

void tearDown(void)
{
    // This function is called **after each test**.
}

static void
rb_init(RingBuffer *rb, void *mem, uint32_t capacity, uint32_t elem_size);
static void test_init_empty(void);
static void test_put_and_pop_one(void);
static void test_fifo_order_no_wrap(void);
static void test_wrap_around_behavior(void);
static void test_peek_at_and_bounds(void);
static void test_pop_ptr_returns_internal_address(void);
static void test_wrap_index_helper(void);
static void test_full_count_requires_isFull_update__KNOWN_ISSUE(void);
static void
test_overflow_increments_elementCount_past_capacity__KNOWN_ISSUE(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_empty);
    RUN_TEST(test_put_and_pop_one);
    RUN_TEST(test_fifo_order_no_wrap);
    RUN_TEST(test_wrap_around_behavior);
    RUN_TEST(test_peek_at_and_bounds);
    RUN_TEST(test_pop_ptr_returns_internal_address);
    RUN_TEST(test_wrap_index_helper);

    /* Known issues: skipped until fixed */
    RUN_TEST(test_full_count_requires_isFull_update__KNOWN_ISSUE);
    RUN_TEST(test_overflow_increments_elementCount_past_capacity__KNOWN_ISSUE);

    return UNITY_END();
}

static void
rb_init(RingBuffer *rb, void *mem, uint32_t capacity, uint32_t elem_size)
{
    rb->startAddress = mem;
    rb->head         = 0;
    rb->tail         = 0;
    rb->bufferLength = capacity;
    rb->elementSize  = elem_size;
    rb->stride       = elem_size;
    rb->elementCount = 0;
    rb->isFull       = 0; /* Note: implementation never updates this */
    memset(mem, 0, capacity * elem_size);
}

static void test_init_empty(void)
{
    RingBuffer rb;
    int mem[8];
    rb_init(&rb, mem, 8, sizeof(int));

    TEST_ASSERT_EQUAL_UINT32(0, rb.head);
    TEST_ASSERT_EQUAL_UINT32(0, rb.tail);
    TEST_ASSERT_EQUAL_UINT32(0, rb.elementCount);
    TEST_ASSERT_EQUAL_UINT32(0, ring_buffer_count(&rb));

    /* pop on empty */
    int out = -1;
    TEST_ASSERT_EQUAL_UINT32(1, ring_buffer_pop(&rb, &out));
    TEST_ASSERT_NULL(ring_buffer_pop_ptr(&rb));

    /* peek_at on empty */
    TEST_ASSERT_NULL(ring_buffer_peek_at(&rb, 0));
}

static void test_put_and_pop_one(void)
{
    RingBuffer rb;
    int mem[4];
    rb_init(&rb, mem, 4, sizeof(int));

    int v = 42, out = -1;
    ring_buffer_put(&rb, &v);

    TEST_ASSERT_EQUAL_UINT32(1, rb.elementCount);
    TEST_ASSERT_EQUAL_UINT32(1, ring_buffer_count(&rb));

    TEST_ASSERT_EQUAL_UINT32(0, ring_buffer_pop(&rb, &out));
    TEST_ASSERT_EQUAL_INT(42, out);
    TEST_ASSERT_EQUAL_UINT32(0, rb.elementCount);
    TEST_ASSERT_EQUAL_UINT32(0, ring_buffer_count(&rb));
}

static void test_fifo_order_no_wrap(void)
{
    RingBuffer rb;
    int mem[8];
    rb_init(&rb, mem, 8, sizeof(int));

    for (int i = 1; i <= 5; ++i)
    {
        ring_buffer_put(&rb, &i);
    }
    TEST_ASSERT_EQUAL_UINT32(5, rb.elementCount);
    TEST_ASSERT_EQUAL_UINT32(5, ring_buffer_count(&rb));

    for (int i = 1; i <= 5; ++i)
    {
        int out = 0;
        TEST_ASSERT_EQUAL_UINT32(0, ring_buffer_pop(&rb, &out));
        TEST_ASSERT_EQUAL_INT(i, out);
    }
    TEST_ASSERT_EQUAL_UINT32(0, rb.elementCount);
    TEST_ASSERT_EQUAL_UINT32(0, ring_buffer_count(&rb));
}

static void test_wrap_around_behavior(void)
{
    RingBuffer rb;
    int mem[4];
    rb_init(&rb, mem, 4, sizeof(int));

    int a = 1, b = 2, c = 3, d = 4, e = 5, out = 0;
    ring_buffer_put(&rb, &a);
    ring_buffer_put(&rb, &b);
    TEST_ASSERT_EQUAL_UINT32(2, rb.head);

    TEST_ASSERT_EQUAL_UINT32(0, ring_buffer_pop(&rb, &out));
    TEST_ASSERT_EQUAL_INT(1, out);
    TEST_ASSERT_EQUAL_UINT32(1, rb.tail);

    ring_buffer_put(&rb, &c);
    ring_buffer_put(&rb, &d);
    ring_buffer_put(&rb, &e); /* causes head wrap */

    TEST_ASSERT_EQUAL_UINT32(4, rb.elementCount); /* equals capacity */

    int expect[] = {2, 3, 4, 5};
    for (int i = 0; i < 4; ++i)
    {
        TEST_ASSERT_EQUAL_UINT32(0, ring_buffer_pop(&rb, &out));
        TEST_ASSERT_EQUAL_INT(expect[i], out);
    }
    TEST_ASSERT_EQUAL_UINT32(0, rb.elementCount);
}

static void test_peek_at_and_bounds(void)
{
    RingBuffer rb;
    int mem[5];
    rb_init(&rb, mem, 5, sizeof(int));

    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; ++i)
    {
        ring_buffer_put(&rb, &vals[i]);
    }

    TEST_ASSERT_NOT_NULL(ring_buffer_peek_at(&rb, 0));
    TEST_ASSERT_NOT_NULL(ring_buffer_peek_at(&rb, 1));
    TEST_ASSERT_NOT_NULL(ring_buffer_peek_at(&rb, 2));

    TEST_ASSERT_EQUAL_INT(10, *(int *)ring_buffer_peek_at(&rb, 0));
    TEST_ASSERT_EQUAL_INT(20, *(int *)ring_buffer_peek_at(&rb, 1));
    TEST_ASSERT_EQUAL_INT(30, *(int *)ring_buffer_peek_at(&rb, 2));

    TEST_ASSERT_NULL(ring_buffer_peek_at(&rb, 3));

    int out = 0;
    (void)ring_buffer_pop(&rb, &out); /* remove 10 */
    TEST_ASSERT_EQUAL_INT(20, *(int *)ring_buffer_peek_at(&rb, 0));
}

static void test_pop_ptr_returns_internal_address(void)
{
    RingBuffer rb;
    int mem[4];
    rb_init(&rb, mem, 4, sizeof(int));

    int a = 111, b = 222;
    ring_buffer_put(&rb, &a);
    ring_buffer_put(&rb, &b);

    void *p0 = ring_buffer_pop_ptr(&rb);
    TEST_ASSERT_EQUAL_PTR(&mem[0], p0);
    TEST_ASSERT_EQUAL_INT(111, *(int *)p0);

    void *p1 = ring_buffer_pop_ptr(&rb);
    TEST_ASSERT_EQUAL_PTR(&mem[1], p1);
    TEST_ASSERT_EQUAL_INT(222, *(int *)p1);

    TEST_ASSERT_NULL(ring_buffer_pop_ptr(&rb));
}

static void test_wrap_index_helper(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, ring_buffer_wrap_index(0, 4));
    TEST_ASSERT_EQUAL_UINT32(1, ring_buffer_wrap_index(5, 4));
    TEST_ASSERT_EQUAL_UINT32(0, ring_buffer_wrap_index(8, 4));
    TEST_ASSERT_EQUAL_UINT32(3, ring_buffer_wrap_index(7, 4));
}

/* ---------- Tests that currently expose defects (ignored/skipped) ---------- */

static void test_full_count_requires_isFull_update__KNOWN_ISSUE(void)
{
    RingBuffer rb;
    int mem[4];
    rb_init(&rb, mem, 4, sizeof(int));

    int v = 0;
    for (int i = 0; i < 4; i++)
    {
        v = i;
        ring_buffer_put(&rb, &v);
    }

    TEST_ASSERT_EQUAL_UINT32(4, ring_buffer_count(&rb));
}

static void
test_overflow_increments_elementCount_past_capacity__KNOWN_ISSUE(void)
{
    RingBuffer rb;
    int mem[3];
    rb_init(&rb, mem, 3, sizeof(int));

    int v = 0;
    for (int i = 0; i < 4; i++)
    {
        v = i + 1;
        ring_buffer_put(&rb, &v);
    } /* overflow by one */

    TEST_ASSERT_LESS_OR_EQUAL_UINT32(rb.bufferLength, rb.elementCount);
}
