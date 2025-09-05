#include <pg_doctest.hpp>

#include "apex_omni_shmem.h"

struct api_fixture {
  api_fixture() {
    apex_find_omni_shmem_api(&consumer);
    api = static_cast<decltype(api)>(consumer.ctx);
  }

protected:
  omni_shmem_api_apex_consumer_t consumer{
      .on_available = [](omni_shmem_api_apex_t *api, auto c) { c->ctx = api->api; }};
  omni_shmem_api *api;
};

DOCTEST_TEST_CASE_FIXTURE(api_fixture, "Open or create (transient)") {
  {
    auto arena = api->open_or_create("_omni_shmem_test_", 64 * 1024, true);
    auto alloc = arena->find_or_construct(arena, "test", sizeof("test"), nullptr, nullptr);
    DOCTEST_CHECK(!alloc.found);
    DOCTEST_CHECK(alloc.ptr);
    arena->release(arena);
  }
}

DOCTEST_TEST_CASE_FIXTURE(api_fixture, "Find or construct") {
  {
    auto arena = api->open_or_create("_omni_shmem_test_", 64 * 1024, true);
    auto alloc =
        arena->find_or_construct(arena, "test_find_or_construct", sizeof("test"), nullptr, nullptr);
    DOCTEST_CHECK(!alloc.found);
    DOCTEST_CHECK(alloc.ptr);

    auto alloc1 =
        arena->find_or_construct(arena, "test_find_or_construct", sizeof("test"), nullptr, nullptr);
    DOCTEST_CHECK(alloc1.found);
    DOCTEST_CHECK(alloc1.ptr);

    DOCTEST_CHECK_EQ(alloc.ptr, alloc1.ptr);

    arena->release(arena);
  }
}

DOCTEST_TEST_CASE_FIXTURE(api_fixture, "Find") {
  {
    auto arena = api->open_or_create("_omni_shmem_test_", 64 * 1024, true);
    auto alloc0 = arena->find(arena, "test_find");
    DOCTEST_CHECK(!alloc0);

    auto alloc = arena->find_or_construct(arena, "test_find", sizeof("test"), nullptr, nullptr);
    DOCTEST_CHECK(!alloc.found);
    DOCTEST_CHECK(alloc.ptr);

    auto alloc1 = arena->find(arena, "test_find");
    DOCTEST_CHECK_EQ(alloc1, alloc.ptr);

    arena->release(arena);
  }
}

DOCTEST_TEST_CASE_FIXTURE(api_fixture, "Destroy") {
  {
    auto arena = api->open_or_create("_omni_shmem_test_", 64 * 1024, true);

    auto alloc = arena->find_or_construct(arena, "test_delete", sizeof("test"), nullptr, nullptr);
    DOCTEST_CHECK(!alloc.found);
    DOCTEST_CHECK(alloc.ptr);

    DOCTEST_CHECK(arena->destroy(arena, "test_delete"));
    DOCTEST_CHECK(!arena->destroy(arena, "test_delete"));

    auto alloc1 = arena->find(arena, "test_find");
    DOCTEST_CHECK(!alloc1);

    arena->release(arena);
  }
}

DOCTEST_TEST_CASE_FIXTURE(api_fixture, "Destroying with a destructor") {
  {
    auto arena = api->open_or_create("_omni_shmem_test_", 64 * 1024, true);
    void *destructor_called = nullptr;
    auto alloc = arena->find_or_construct(
        arena, "destructor", sizeof("test"),
        [](void *ptr, void *ctx) { *static_cast<void **>(ctx) = ptr; }, &destructor_called);
    DOCTEST_CHECK(!alloc.found);
    DOCTEST_CHECK(alloc.ptr);

    DOCTEST_CHECK(arena->destroy(arena, "destructor"));
    DOCTEST_CHECK_EQ(destructor_called, alloc.ptr);

    destructor_called = nullptr;
    DOCTEST_CHECK(!arena->destroy(arena, "destructor"));
    DOCTEST_CHECK(!destructor_called);

    arena->release(arena);
  }
}
