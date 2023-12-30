cmake_minimum_required(VERSION 3.25.1)
project(omni_ext_test)

include(CPM)
include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_ext_test
        SCHEMA omni_ext_test
        PRIVATE true
        RELOCATABLE false
        SOURCES omni_ext_test.c
        REQUIRES omni_ext
        VERSION 0.1)

add_postgresql_extension(
        omni_ext_test_no_preload
        SCHEMA omni_ext_test_no_preload
        PRIVATE true
        RELOCATABLE false
        SOURCES omni_ext_test.c
        REQUIRES omni_ext
        VERSION 0.1)

target_link_libraries(omni_ext_test dynpgext)

target_link_libraries(omni_ext_test_no_preload dynpgext)
target_compile_definitions(omni_ext_test_no_preload PUBLIC NO_PRELOAD)

add_dependencies(omni_ext_test omni_ext)
add_dependencies(omni_ext_test_no_preload omni_ext)