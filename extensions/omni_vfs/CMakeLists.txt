cmake_minimum_required(VERSION 3.25.1)
project(omni_vfs)

include(CTest)
include(FindPkgConfig)
include(FindThreads)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_vfs
        VERSION 0.1
        SCHEMA omni_vfs
        RELOCATABLE false
        DEPENDS_ON libpgaug
        SCRIPTS omni_vfs--0.1.sql
        SOURCES omni_vfs.c local_fs.c)

target_link_libraries(omni_vfs libpgaug)
