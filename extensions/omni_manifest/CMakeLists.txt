cmake_minimum_required(VERSION 3.25.1)
project(omni_manifest)

include(CTest)
include(FindPkgConfig)
include(FindThreads)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_manifest_test_1
        VERSION 1
        PRIVATE ON
        TESTS OFF)

add_postgresql_extension(
        omni_manifest_test_2
        REQUIRES omni_manifest_test_1
        VERSION 1
        PRIVATE ON
        TESTS OFF)

add_postgresql_extension(
        omni_manifest
        SCHEMA omni_manifest
        TESTS_REQUIRE omni_manifest_test_1 omni_manifest_test_2
        RELOCATABLE false)