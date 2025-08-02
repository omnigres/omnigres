cmake_minimum_required(VERSION 3.25.1)
project(omni_northwind)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_northwind
        SCHEMA omni_northwind
        COMMENT "Northwind sample database"
        REQUIRES omni_httpc
        TESTS_REQUIRE omni_test
        RELOCATABLE false)
