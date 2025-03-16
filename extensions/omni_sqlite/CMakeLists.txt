cmake_minimum_required(VERSION 3.25.1)
project(omni_sqlite)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_sqlite
        SOURCES ${CMAKE_CURRENT_LIST_DIR}/../../deps/sqlite-3.48.0/sqlite3.c
        omni_sqlite.cpp sqlite.cpp sqlite3_db_dump.c
        SCHEMA omni_sqlite
        RELOCATABLE false)

target_compile_features(omni_sqlite PUBLIC cxx_std_20)

target_include_directories(omni_sqlite PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../deps/sqlite-3.48.0/)
