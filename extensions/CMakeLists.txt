cmake_minimum_required(VERSION 3.25.1)
project(omnigres)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

enable_testing()

add_subdirectory(omni_ext)
add_subdirectory(omni_containers)
add_subdirectory(omni_httpd)
add_subdirectory(omni_sql)
add_subdirectory(omni_web)
