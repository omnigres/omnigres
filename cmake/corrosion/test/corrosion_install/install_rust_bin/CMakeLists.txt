cmake_minimum_required(VERSION 3.22)
project(test_project VERSION 0.1.0)

# Note: Corrosion supports `hostbuild`, so building a Rust binary in a subproject
# like this doesn't offer any benefit over using the hostbuild option.
# However, this is a reasonable way to test that installing Rust binaries via
# corrosion_install works as expected.
include(ExternalProject)

set(bin_suffix "")
if(CMAKE_HOST_WIN32)
    set(bin_suffix ".exe")
endif()
set(generator_bin_path "${CMAKE_CURRENT_BINARY_DIR}/rust_bin/bin/my_rust_bin${bin_suffix}")

ExternalProject_Add(
        rust_bin
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/rust_bin"
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rust_bin"
        CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/rust_bin"
)

# This custom command is the main part of the test:
# We test that corrosion (in the CMake of the ExternalProject) properly installed
# a Rust executable to the location we specified by running the executable, which generates some cpp code.
add_custom_command(
        OUTPUT generated_main.cpp
        COMMAND "${generator_bin_path}" > "${CMAKE_CURRENT_BINARY_DIR}/generated_main.cpp"
        DEPENDS rust_bin
)

add_executable(generated_from_installed_bin ${CMAKE_CURRENT_BINARY_DIR}/generated_main.cpp)
