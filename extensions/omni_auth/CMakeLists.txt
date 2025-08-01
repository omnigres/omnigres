cmake_minimum_required(VERSION 3.25.1)
project(omni_json)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)
include(OpenSSL)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../cmake/corrosion ${CMAKE_BINARY_DIR}/corrosion)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_auth
        COMMENT "Basic session management"
        SCHEMA omni_auth
        REQUIRES omni_types omni_id pgcrypto btree_gist omni_polyfill
        SOURCES webauthn.cpp
        RELOCATABLE false)

corrosion_import_crate(MANIFEST_PATH libwebauthn/Cargo.toml)

target_link_libraries(omni_auth PUBLIC libwebauthn crypto ssl)

target_compile_features(omni_auth PUBLIC cxx_std_20)

target_include_directories(omni_auth PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/deps
        ${CMAKE_CURRENT_LIST_DIR}/../../deps/fmt/include)

