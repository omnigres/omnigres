cmake_minimum_required(VERSION 3.25.1)
project(omni_python)

include(CTest)
include(CheckIncludeFile)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_python
        VERSION 0.1
        SCHEMA omni_python
        RELOCATABLE false
        SCRIPTS omni_python--0.1.sql
        REQUIRES plpython3u
)