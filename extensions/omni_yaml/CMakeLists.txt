cmake_minimum_required(VERSION 3.25.1)
project(omni_yaml)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

CPMAddPackage(NAME libfyaml SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../deps/libfyaml VERSION ${VERSION_libfyaml}
        EXCLUDE_FROM_ALL YES OPTIONS "BUILD_SHARED_LIBS OFF")

add_postgresql_extension(
        omni_yaml
        SCHEMA omni_yaml
        SOURCES omni_yaml.c
        RELOCATABLE false)

target_link_libraries(omni_yaml PRIVATE fyaml)