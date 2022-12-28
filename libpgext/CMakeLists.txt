cmake_minimum_required(VERSION 3.25.1)
project(libpgext)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_library(libpgext STATIC libpgext.c)
set_property(TARGET libpgext PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(libpgext
        PRIVATE ${PostgreSQL_SERVER_INCLUDE_DIRS}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
