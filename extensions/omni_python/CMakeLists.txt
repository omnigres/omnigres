cmake_minimum_required(VERSION 3.25.1)
project(omni_python)

include(CTest)
include(CheckIncludeFile)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_python
        COMMENT "First-class Python support"
        SCHEMA omni_python
        RELOCATABLE false
        REQUIRES plpython3u
        SOURCES omni_python.c
        TESTS_REQUIRE omni_httpd
)