cmake_minimum_required(VERSION 3.25.1)
project(libgluepg_yyjson)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)
find_package(Yyjson REQUIRED)

add_library(libgluepg_yyjson STATIC libgluepg_yyjson.c)
set_property(TARGET libgluepg_yyjson PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(libgluepg_yyjson
        PRIVATE ${PostgreSQL_SERVER_INCLUDE_DIRS} "${yyjson_BINARY_DIR}/src"
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libgluepg_yyjson PUBLIC yyjson libpgaug)