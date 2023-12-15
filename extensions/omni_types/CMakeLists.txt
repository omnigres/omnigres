cmake_minimum_required(VERSION 3.25.1)
project(omni_types)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

include(CPM)
include(CTest)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_types
        SCHEMA omni_types
        RELOCATABLE false
        SOURCES omni_types.c unit.c sum_type.c
)

add_library(libomnitypes INTERFACE)
target_include_directories(libomnitypes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
