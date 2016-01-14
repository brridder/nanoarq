#include "nanoarq_in_test_project.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(lin_alloc) {};

namespace
{

TEST(lin_alloc, init_sets_fields)
{
    void *base = (void *)0x12345678;
    int capacity = 3141592;
    arq__lin_alloc_t a;
    arq__lin_alloc_init(&a, base, capacity);

    CHECK_EQUAL(base, a.base);
    CHECK_EQUAL(base, a.top);
    CHECK_EQUAL(capacity, a.capacity);
}

struct Fixture
{
    Fixture() { arq__lin_alloc_init(&a, mem, sizeof(mem)); }
    arq__lin_alloc_t a;
    char mem[1024];
};

TEST(lin_alloc, first_alloc_align_1_returns_base)
{
    Fixture f;
    void *p = arq__lin_alloc_alloc(&f.a, 8, 1);
    CHECK_EQUAL(f.mem, p);
}

TEST(lin_alloc, second_alloc_align_1_returns_top)
{
    Fixture f;
    arq__lin_alloc_alloc(&f.a, 16, 1);
    void *p = arq__lin_alloc_alloc(&f.a, 1, 1);
    CHECK_EQUAL((void *)&f.mem[16], p);
}

TEST(lin_alloc, pads_for_alignment)
{
    Fixture f;
    f.a.base = (char *)(0);
    f.a.top = (char *)(1);
    void *p = arq__lin_alloc_alloc(&f.a, 1, 4);
    CHECK_EQUAL((void *)4, p);
}

TEST(lin_alloc, doesnt_assert_when_perfectly_full)
{
    struct Local
    {
        static bool& called() { static bool c = false; return c; }
        static void assert_cb(char const *, int, char const *, char const *) { called() = true; }
    };

    arq_assert_cb_t orig;
    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_get_assert_handler(&orig));
    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_set_assert_handler(&Local::assert_cb));

    Fixture f;
    arq__lin_alloc_alloc(&f.a, f.a.capacity, 1);
    CHECK(Local::called() == false);

    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_set_assert_handler(orig));
}

TEST(lin_alloc, asserts_when_capacity_exhausted)
{
    struct Local
    {
        static bool& called() { static bool c = false; return c; }
        static void assert_cb(char const *, int, char const *, char const *) { called() = true; }
    };

    arq_assert_cb_t orig;
    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_get_assert_handler(&orig));
    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_set_assert_handler(&Local::assert_cb));

    Fixture f;
    arq__lin_alloc_alloc(&f.a, f.a.capacity + 1, 1);
    CHECK(Local::called());

    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_set_assert_handler(orig));
}

TEST(lin_alloc, alloc_returns_null_when_asserts_disabled)
{
    struct Local
    {
        static void assert_cb(char const *, int, char const *, char const *) {}
    };

    arq_assert_cb_t orig;
    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_get_assert_handler(&orig));
    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_set_assert_handler(&Local::assert_cb));

    Fixture f;
    CHECK_EQUAL(NULL, arq__lin_alloc_alloc(&f.a, f.a.capacity + 1, 1));

    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_set_assert_handler(orig));
}

}

