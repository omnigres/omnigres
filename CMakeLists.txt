cmake_minimum_required(VERSION 3.25.1)
project(omnigres)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

enable_testing()

add_subdirectory(dynpgext)
add_subdirectory(libpgaug)
add_subdirectory(libgluepg_curl)
add_subdirectory(libgluepg_yyjson)
add_subdirectory(libgluepg_stc)
add_subdirectory(extensions)