cmake_minimum_required(VERSION 3.25.1)
project(omni_test)

include(CPM)
include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_test
        SCHEMA omni_test
        PRIVATE ON
        RELOCATABLE false
        SOURCES omni_test.c
        REQUIRES omni
        VERSION 1)

add_postgresql_extension(
        omni_test
        TARGET omni_test_v2
        SCHEMA omni_test
        PRIVATE ON
        RELOCATABLE false
        SOURCES omni_test.c
        UPGRADE_SCRIPTS tests/omni_test--1--2.sql
        NO_DEFAULT_CONTROL ON
        REQUIRES omni
        VERSION 2
        TESTS OFF)


target_link_libraries(omni_test libomni)
target_link_libraries(omni_test_v2 libomni)

#add_dependencies(omni_test omni)