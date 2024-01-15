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
        omni_manifest_test_1
        TARGET omni_manifest_test_1_2
        UPGRADE_SCRIPTS tests/omni_manifest_test_1--1--2.sql
        NO_DEFAULT_CONTROL ON
        VERSION 2
        PRIVATE ON
        TESTS OFF)

add_postgresql_extension(
        omni_manifest_test_2
        REQUIRES omni_manifest_test_1
        TARGET omni_manifest_test_2_2
        UPGRADE_SCRIPTS tests/omni_manifest_test_2--1--2.sql
        NO_DEFAULT_CONTROL ON
        VERSION 2
        PRIVATE ON
        TESTS OFF)

add_postgresql_extension(
        omni_manifest
        SCHEMA omni_manifest
        TESTS_REQUIRE omni_manifest_test_1 omni_manifest_test_2 omni_manifest_test_1_2 omni_manifest_test_2_2
        RELOCATABLE false)