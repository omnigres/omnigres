cmake_minimum_required(VERSION 3.0.2)
project(metalang99 LANGUAGES C)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
