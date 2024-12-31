cmake_minimum_required(VERSION 3.25.1)
project(omni_id)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

include(CPM)
include(CTest)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_rest
        SCHEMA omni_rest
        REQUIRES omni_httpd omni_sql omni_web omni_var pgcrypto
)