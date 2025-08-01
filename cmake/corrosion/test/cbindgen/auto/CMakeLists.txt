cmake_minimum_required(VERSION 3.15)
project(test_project VERSION 0.1.0)

set(CORROSION_TOOLS_RUST_TOOLCHAIN "stable")
include(../../test_header.cmake)
corrosion_import_crate(MANIFEST_PATH rust/Cargo.toml)
corrosion_experimental_cbindgen(TARGET the_rust_lib_crate_name HEADER_NAME "rust-lib.h")

add_executable(cpp-exe main.cpp)
set_property(TARGET cpp-exe PROPERTY CXX_STANDARD 11)
target_link_libraries(cpp-exe PUBLIC the_rust_lib_crate_name)
