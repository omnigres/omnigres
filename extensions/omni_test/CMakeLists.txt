cmake_minimum_required(VERSION 3.25.1)
project(omni_test)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_test
        SCHEMA omni_test
        VERSION 0.4.0
        COMMENT "Testing framework"
        REQUIRES dblink omni_cloudevents
        TESTS_REQUIRE omni_schema omni_httpd
        RELOCATABLE false)
