cmake_minimum_required(VERSION 3.25.1)
project(omni_python)

include(CTest)
include(CheckIncludeFile)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_python
        SCHEMA omni_python
        RELOCATABLE false
        REQUIRES plpython3u
        TESTS_REQUIRE omni_httpd
)