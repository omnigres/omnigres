cmake_minimum_required(VERSION 3.25.1)
project(dynpgext_test)

include(CPM)
include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        dynpgext_test
        VERSION 0.1
        SCRIPTS dynpgext_test--0.1.sql
        RELOCATABLE false
        SOURCES dynpgext_test.c
        REGRESS loader_present)

target_link_libraries(dynpgext_test dynpgext)