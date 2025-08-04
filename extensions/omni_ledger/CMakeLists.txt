cmake_minimum_required(VERSION 3.25.1)
project(omni_id)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

include(CPM)
include(CTest)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_ledger
        VERSION 0.1.3
        COMMENT "Financial ledger"
        SCHEMA omni_ledger
        SOURCES omni_ledger.c
        REQUIRES omni_id omni_polyfill
        TESTS_REQUIRE omni_var
        DEPENDS_ON omni
)

target_link_libraries(omni_ledger libomni)