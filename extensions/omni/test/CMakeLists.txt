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
        PRIVATE TRUE
        RELOCATABLE false
        SOURCES omni_test.c
        REQUIRES omni
        VERSION 1)


target_link_libraries(omni_test libomni)

#add_dependencies(omni_test omni)