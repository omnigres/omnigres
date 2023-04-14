cmake_minimum_required(VERSION 3.25)
project(inja)

add_executable(inja inja.cpp)
target_include_directories(inja PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
set_property(TARGET inja PROPERTY CXX_STANDARD 20)

install(TARGETS inja RUNTIME DESTINATION bin)