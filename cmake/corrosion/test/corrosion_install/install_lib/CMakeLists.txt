cmake_minimum_required(VERSION 3.15)
project(test_project VERSION 0.1.0)
include(ExternalProject)

add_library(static_lib STATIC IMPORTED)
add_library(shared_lib SHARED IMPORTED)
set(install_prefix "${CMAKE_CURRENT_BINARY_DIR}/rust_lib")
set(static_lib_install_path "${install_prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}rust_lib${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(shared_lib_install_path "${install_prefix}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}rust_lib${CMAKE_SHARED_LIBRARY_SUFFIX}")


set_target_properties(static_lib PROPERTIES
                      IMPORTED_LOCATION
                      "${static_lib_install_path}")

set_target_properties(shared_lib PROPERTIES
                      IMPORTED_LOCATION
                      "${shared_lib_install_path}")

add_executable(main-static main.cpp)
target_link_libraries(main-static PRIVATE static_lib)

ExternalProject_Add(
        rust_lib
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/rust_lib"
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rust_lib"
        CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${install_prefix}"
        # INSTALL_BYPRODUCTS "${static_lib_install_path}"
)

# Dummy target since INSTALL_BYPRODUCTS requires CMake 3.26
add_custom_target(build_rust_project_dummy
                  COMMAND echo dummy
                  BYPRODUCTS "${static_lib_install_path}" "${shared_lib_install_path}"
                  DEPENDS rust_lib)

add_dependencies(main-static build_rust_project_dummy)

set(CMAKE_BUILD_RPATH ${install_prefix}/lib)
add_executable(main-shared main.cpp)
target_link_libraries(main-shared
        PUBLIC shared_lib)

add_dependencies(main-shared build_rust_project_dummy)
