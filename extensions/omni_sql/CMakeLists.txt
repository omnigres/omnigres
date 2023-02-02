cmake_minimum_required(VERSION 3.25.1)
project(omni_sql)

include(CPM)
include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_sql
        VERSION 0.1
        SCHEMA omni_sql
        RELOCATABLE false
        SCRIPTS omni_sql--0.1.sql
        SOURCES omni_sql.c deparse.c deparse_15.c deparse_14.c deparse_13.c
        REGRESS deparse cte
)

target_link_libraries(omni_sql libpgaug)