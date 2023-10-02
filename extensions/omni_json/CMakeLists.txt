cmake_minimum_required(VERSION 3.25.1)
project(omni_json)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_json
        VERSION 0.1
        SCHEMA omni_json
        RELOCATABLE false
        SCRIPTS omni_json--0.1.sql)