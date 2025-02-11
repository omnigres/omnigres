cmake_minimum_required(VERSION 3.25.1)
project(omni_schema)

include(CTest)
include(FindPkgConfig)
include(FindThreads)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_schema
        COMMENT "Advanced schema management tooling"
        SCHEMA omni_schema
        RELOCATABLE false
        REQUIRES omni_sql omni_vfs omni_polyfill omni_yaml dblink postgres_fdw omni_types
        TESTS_REQUIRE omni_python)