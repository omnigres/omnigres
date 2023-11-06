cmake_minimum_required(VERSION 3.25.1)
project(omni_vfs)

include(CTest)
include(FindPkgConfig)
include(FindThreads)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_vfs_types_v1
        SCHEMA omni_vfs_types_v1
        RELOCATABLE false
        TESTS OFF)

add_postgresql_extension(
        omni_vfs
        SCHEMA omni_vfs
        RELOCATABLE false
        DEPENDS_ON libpgaug
        REQUIRES omni_vfs_types_v1
        SOURCES omni_vfs.c local_fs.c pg_path_v15.c)

target_link_libraries(omni_vfs libpgaug)
