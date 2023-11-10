cmake_minimum_required(VERSION 3.25.1)
project(omni_containers)

include(CTest)
include(CheckIncludeFile)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_containers
        SCHEMA omni_containers
        RELOCATABLE false
        SOURCES omni_containers.c init.c
        DEPENDS_ON dynpgext omni_ext
        REQUIRES omni_httpc omni_web)

target_compile_definitions(omni_containers PUBLIC ${CMAKE_C_BYTE_ORDER})
target_link_libraries(omni_containers dynpgext)