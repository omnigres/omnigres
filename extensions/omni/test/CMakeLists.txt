cmake_minimum_required(VERSION 3.25.1)
project(omni__test)

include(CPM)
include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

set(SOURCES omni__test.c hooks.c)

add_postgresql_extension(
        omni__test
        SCHEMA omni__test
        PRIVATE ON
        RELOCATABLE false
        SOURCES ${SOURCES}
        TESTS_REQUIRE omni omni_types
        VERSION 1)

add_postgresql_extension(
        omni__test
        TARGET omni__test_v2
        SCHEMA omni__test
        PRIVATE ON
        RELOCATABLE false
        SOURCES ${SOURCES}
        UPGRADE_SCRIPTS tests/omni__test--1--2.sql
        NO_DEFAULT_CONTROL ON
        TESTS_REQUIRE omni omni_types
        VERSION 2
        TESTS OFF)


add_postgresql_extension(
        no_fun
        SCHEMA omni__test
        PRIVATE ON
        RELOCATABLE false
        SOURCES no_fun.c
        TESTS_REQUIRE omni
        VERSION 1)


target_link_libraries(omni__test libomni)
target_link_libraries(omni__test_v2 libomni)
target_link_libraries(no_fun libomni)

#add_dependencies(omni__test omni)