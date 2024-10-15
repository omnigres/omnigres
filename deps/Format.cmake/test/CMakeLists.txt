        cmake_minimum_required( VERSION
3.16)
project(                                    FormatTest)
add_subdirectory(
..
                                ${CMAKE_CURRENT_BINARY_DIR}/format)
                    add_executable(             test        test.cpp )
