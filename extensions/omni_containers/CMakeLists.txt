cmake_minimum_required(VERSION 3.25.1)
project(omni_containers)

include(CTest)
include(CheckIncludeFile)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

check_include_file("byteswap.h" HAVE_BYTESWAP_H)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_containers
        VERSION 0.1
        SCHEMA omni_containers
        RELOCATABLE false
        SCRIPTS omni_containers--0.1.sql
        SOURCES omni_containers.c init.c
        REGRESS images container)

target_compile_definitions(omni_containers PUBLIC ${CMAKE_C_BYTE_ORDER})
target_link_libraries(omni_containers libgluepg_curl libgluepg_yyjson libpgaug dynpgext)