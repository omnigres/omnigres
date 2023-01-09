cmake_minimum_required(VERSION 3.25.1)
project(libgluepg_stc)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)
find_package(STC REQUIRED)

add_library(libgluepg_stc INTERFACE)
target_include_directories(libgluepg_stc
        INTERFACE ${PostgreSQL_SERVER_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libgluepg_stc INTERFACE stc libpgaug)