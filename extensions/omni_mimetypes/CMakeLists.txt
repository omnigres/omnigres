cmake_minimum_required(VERSION 3.25.1)
project(omni_mimetypes)

include(CTest)
include(FindPkgConfig)
include(FindThreads)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

add_postgresql_extension(
        omni_mimetypes
        VERSION 0.1
        SCHEMA omni_mimetypes
        RELOCATABLE false
        SCRIPTS omni_mimetypes--0.1.sql)