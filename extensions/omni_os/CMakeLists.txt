cmake_minimum_required(VERSION 3.25.1)
project(omni_os)

include(CTest)
include(CheckIncludeFile)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_os
        SCHEMA omni_os
        RELOCATABLE false
        SUPERUSER true
        SOURCES omni_os.c)