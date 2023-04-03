cmake_minimum_required(VERSION 3.25.1)
project(libpgaug_test)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        libpgaug_test
        VERSION 0.1
        PRIVATE true
        SCRIPTS libpgaug_test--0.1.sql
        SOURCES libpgaug_test.c
        REGRESS memcxt)

target_link_libraries(libpgaug_test libpgaug)