cmake_minimum_required(VERSION 3.25.1)
project(omni_sqlite)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13.1)
        message(FATAL_ERROR "GCC version must be at least 13.1 Detected version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif ()
endif ()

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_sqlite
        COMMENT "Embedded SQLite"
        SOURCES ${CMAKE_CURRENT_LIST_DIR}/../../deps/sqlite-3.48.0/sqlite3.c
        omni_sqlite.cpp sqlite.cpp sqlite3_db_dump.c
        SCHEMA omni_sqlite
        RELOCATABLE false)

target_compile_features(omni_sqlite PUBLIC cxx_std_20)

target_include_directories(omni_sqlite PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/deps
        ${CMAKE_CURRENT_LIST_DIR}/../../deps/sqlite-3.48.0/
        ${CMAKE_CURRENT_LIST_DIR}/../../deps/fmt/include)
