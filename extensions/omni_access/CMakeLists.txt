cmake_minimum_required(VERSION 3.25.1)
project(omni_access)

include(CTest)
include(FindPkgConfig)
include(FindThreads)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_access
        SCHEMA omni_access
        RELOCATABLE false)