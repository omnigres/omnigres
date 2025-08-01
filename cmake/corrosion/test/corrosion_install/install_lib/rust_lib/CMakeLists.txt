cmake_minimum_required(VERSION 3.15)
project(test_project VERSION 0.1.0)
include(../../../test_header.cmake)

corrosion_import_crate(MANIFEST_PATH Cargo.toml)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    corrosion_add_target_local_rustflags(rust_lib "-Clink-arg=-Wl,-soname,librust_lib.so")
    set_target_properties(rust_lib-shared PROPERTIES IMPORTED_SONAME librust_lib.so)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    corrosion_add_target_local_rustflags(rust_lib -Clink-arg=-Wl,-install_name,@rpath/librust_lib.dylib,-current_version,1.0,-compatibility_version,1.0)
    set_target_properties(rust_lib-shared PROPERTIES IMPORTED_NO_SONAME 0)
    set_target_properties(rust_lib-shared PROPERTIES IMPORTED_SONAME librust_lib.dylib)
endif()

target_sources(rust_lib INTERFACE include/rust_lib/rust_lib.hpp)

corrosion_install(TARGETS rust_lib)
