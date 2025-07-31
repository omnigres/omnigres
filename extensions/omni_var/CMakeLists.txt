cmake_minimum_required(VERSION 3.25.1)
project(omni_var)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

include(CPM)
include(CTest)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_var
        VERSION 0.3.0
        COMMENT "Scoped variables"
        SCHEMA omni_var
        RELOCATABLE false
        SOURCES omni_var.c txn.c session.c statement.c
)
