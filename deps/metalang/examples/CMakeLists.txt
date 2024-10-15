cmake_minimum_required(VERSION 3.16)
project(examples LANGUAGES C)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wall -Wextra -pedantic -std=c99
                      -ftrack-macro-expansion=0)
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  add_compile_options("-fmacro-backtrace-limit=1")
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  # Enable a standard-conforming C99/C11 preprocessor.
  add_compile_options("/std:c11")
elseif(CMAKE_C_COMPILER_ID STREQUAL "TinyCC")
  add_compile_definitions(ML99_ALLOW_POOR_DIAGNOSTICS)
endif()

include_directories(../include)

add_executable(ackermann ackermann.c)
add_executable(assert_for_each assert_for_each.c)
add_executable(binary_tree binary_tree.c)
add_executable(demo demo.c)
add_executable(duffs_device duffs_device.c)
add_executable(factorial factorial.c)
add_executable(overload overload.c)
add_executable(rectangle rectangle.c)
add_executable(lambda_calculus lambda_calculus.c)

foreach(TARGET ${BUILDSYSTEM_TARGETS})
  set_target_properties(TARGET PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
endforeach()
