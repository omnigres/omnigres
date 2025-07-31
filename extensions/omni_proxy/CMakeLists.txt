cmake_minimum_required(VERSION 3.25.1)
project(omni_proxy)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

include(CPM)
include(CTest)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_proxy
        COMMENT "Proxy HTTP requests from one orb to others."
        SCHEMA omni_proxy
        REQUIRES omni_httpd dblink
)
