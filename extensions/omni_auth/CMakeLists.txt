cmake_minimum_required(VERSION 3.25.1)
project(omni_json)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_auth
        COMMENT "Basic session management"
        SCHEMA omni_auth
        REQUIRES omni_types omni_id pgcrypto btree_gist omni_polyfill
        RELOCATABLE false)