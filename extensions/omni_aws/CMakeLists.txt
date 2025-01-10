cmake_minimum_required(VERSION 3.25.1)
project(omni_aws)

include(CTest)
include(FindPkgConfig)
include(FindThreads)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_aws
        COMMENT "Amazon Web Services APIs (S3)"
        SCHEMA omni_aws
        RELOCATABLE false
        REQUIRES omni_httpc pgcrypto omni_xml omni_web
        TESTS_REQUIRE omni_containers omni_os
        SUPERUSER false
        SHARED_PRELOAD ON)
