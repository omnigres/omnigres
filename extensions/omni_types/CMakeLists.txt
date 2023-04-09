cmake_minimum_required(VERSION 3.25.1)
project(omni_types)

include(CPM)
include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_types
        VERSION 0.1
        SCHEMA omni_types
        RELOCATABLE false
        SCRIPTS omni_types--0.1.sql
        SOURCES sum_type.c
        REGRESS sum_type
)