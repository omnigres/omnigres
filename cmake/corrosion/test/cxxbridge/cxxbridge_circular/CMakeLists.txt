# This CMake project tests the setup for circular dependencies that involve a CXX bridge
# While circular dependencies are usually discouraged, with CXX they are reasonbly natural,
# as ideally, Rust should be able to call C++ freely and vice-versa.
#
# The way this project is set up, the rust_lib target acts as the one target that includes
# the mixed C++ and Rust code with the cpp_lib and cxxbridge targets as implementation details,
# which shouldn't be used individually.
cmake_minimum_required(VERSION 3.24)
project(test_project VERSION 0.1.0 LANGUAGES CXX)
include(../../test_header.cmake)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED 1)

corrosion_import_crate(MANIFEST_PATH rust/Cargo.toml)
corrosion_add_cxxbridge(cxxbridge CRATE rust_lib FILES lib.rs)

if(MSVC)
    set_target_properties(cxxbridge PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

add_library(cpp_lib STATIC cpplib.cpp)
target_include_directories(cpp_lib PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

# Make sure the cpp library can access the headers from the bridge and vice-versa
target_link_libraries(cpp_lib PRIVATE cxxbridge)
target_link_libraries(cxxbridge PRIVATE cpp_lib)

# The 3 libraries (rust, cpp, cxx) have a circular dependency, set this up in the linker
# so that the rust_lib target links to everything and circular references are resolved.
if (CMAKE_CXX_LINK_GROUP_USING_RESCAN_SUPPORTED)
    target_link_libraries(rust_lib INTERFACE
        "$<LINK_GROUP:RESCAN,cxxbridge,rust_lib-static,cpp_lib>")
else()
    target_link_libraries(rust_lib INTERFACE cxxbridge cpp_lib)
endif()

add_executable(cpp_bin main.cpp)
target_link_libraries(cpp_bin rust_lib)

if(MSVC)
    set_target_properties(cpp_lib PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    set_target_properties(cxxbridge PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    set_target_properties(cpp_bin PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()
