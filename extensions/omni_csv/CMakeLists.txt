cmake_minimum_required(VERSION 3.25.1)
project(omni_csv)

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
        omni_csv
        VERSION 0.1.0
        COMMENT "CSV toolkit"
        SOURCES omni_csv.cpp
        SCHEMA omni_csv
        RELOCATABLE false)


target_compile_features(omni_csv PUBLIC cxx_std_20)

target_include_directories(omni_csv PRIVATE
${CMAKE_CURRENT_LIST_DIR}/deps
${CMAKE_CURRENT_LIST_DIR}/../../deps/fmt/include)
