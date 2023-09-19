cmake_minimum_required(VERSION 3.25.1)
project(omni_txn)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

include(CPM)
include(CTest)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_txn
        VERSION 0.1
        SCHEMA omni_txn
        RELOCATABLE false
        SCRIPTS omni_txn--0.1.sql
        SOURCES omni_txn.c
)