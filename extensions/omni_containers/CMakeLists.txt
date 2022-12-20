cmake_minimum_required(VERSION 3.25.1)
project(omni_containers)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

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

target_link_libraries(omni_containers PRIVATE libgluepg_curl libgluepg_yyjson)