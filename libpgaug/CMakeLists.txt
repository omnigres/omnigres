cmake_minimum_required(VERSION 3.25.1)
project(libpgaug)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_library(libpgaug STATIC libpgaug.c)
set_property(TARGET libpgaug PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(libpgaug
        PRIVATE ${PostgreSQL_SERVER_INCLUDE_DIRS}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
