cmake_minimum_required(VERSION 3.25.1)
project(omni_types)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

include(CPM)
include(CTest)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_polyfill
        SCHEMA omni_polyfill
        RELOCATABLE NO
        SOURCES omni_polyfill.c arrays.c uuidv7.c
)