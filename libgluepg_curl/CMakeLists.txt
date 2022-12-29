cmake_minimum_required(VERSION 3.25.1)
project(libgluepg_curl)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

include(FramaC)

enable_testing()

find_package(PostgreSQL REQUIRED)
find_package(CURL REQUIRED)

add_library(libgluepg_curl STATIC libgluepg_curl.c)
set_property(TARGET libgluepg_curl PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(libgluepg_curl
        PRIVATE ${PostgreSQL_SERVER_INCLUDE_DIRS} ${curl_SOURCE_DIR}/include
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libgluepg_curl PUBLIC libcurl libpgaug)
add_framac(TARGET libgluepg_curl FUNCTIONS gluepg_curl_buffer_write_impl)