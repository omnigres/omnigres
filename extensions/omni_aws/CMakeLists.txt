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
        VERSION 1.0.0
        SCHEMA omni_aws
        RELOCATABLE false
        REQUIRES omni_httpc=1.0.0 pgcrypto=* omni_xml=1.0.0 omni_web=1.0.0
        TESTS_REQUIRE omni_containers=1.0.0 omni_os=1.0.0)
