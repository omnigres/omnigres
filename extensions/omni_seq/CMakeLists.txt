cmake_minimum_required(VERSION 3.25.1)
project(omni_seq)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

include(CPM)
include(CTest)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_seq
        SCHEMA omni_seq
        RELOCATABLE false
        SOURCES omni_seq.c
)