cmake_minimum_required(VERSION 3.25.1)
project(omni_sql)

if(NOT DEFINED ENV{OMNI_SQL_ALREADY_INCLUDED})
    set(ENV{OMNI_SQL_ALREADY_INCLUDED} TRUE)

    list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

    include(CPM)
    include(CTest)

    enable_testing()

    find_package(PostgreSQL REQUIRED)

    add_library(libomnisql STATIC
            deparse.c deparse_15.c deparse_14.c deparse_13.c
            lib.c)
    set_property(TARGET libomnisql PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_include_directories(libomnisql
            PRIVATE ${PostgreSQL_SERVER_INCLUDE_DIRS}
            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    add_postgresql_extension(
            omni_sql
            VERSION 0.1
            SCHEMA omni_sql
            RELOCATABLE false
            SCRIPTS omni_sql--0.1.sql
            SOURCES omni_sql.c
            DEPENDS_ON libpgaug
            REGRESS deparse cte parameterized validity
    )

    target_link_libraries(omni_sql libpgaug libomnisql)

endif()