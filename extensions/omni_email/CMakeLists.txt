cmake_minimum_required(VERSION 3.25.1)
project(omni_email)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_email
        SCHEMA omni_email
        COMMENT "E-mail framework"
        REQUIRES omni_id omni_cloudevents omni_polyfill
        RELOCATABLE false)
