cmake_minimum_required(VERSION 3.25.1)
project(omni_web)

include(CTest)
include(FindPkgConfig)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)
find_package(Uriparser REQUIRED)

add_postgresql_extension(
        omni_web
        VERSION 0.1
        SCHEMA omni_web
        RELOCATABLE false
        SCRIPTS omni_web--0.1.sql
        SOURCES omni_web.c urlencode.c)

target_link_libraries(omni_web uriparser)

find_package(Doxygen)

if(DOXYGEN_FOUND)
        set(DOXYGEN_PROJECT_BRIEF "omni_web")

        set(DOXYGEN_GENERATE_TREEVIEW YES)
        set(DOXYGEN_FULL_SIDEBAR NO)

        set(DOXYGEN_OUTPUT_FOR_C YES)
        set(DOXYGEN_MARKDOWN_SUPPORT YES)
        set(DOXYGEN_AUTOLINK_SUPPORT YES)
        set(DOXYGEN_USE_MDFILE_AS_MAINPAGE docs/index.md)
        set(DOXYGEN_EXTRACT_STATIC YES)
        set(DOXYGEN_INLINE_SOURCES YES)
        set(DOXYGEN_REFERENCES_RELATION YES)
        set(DOXYGEN_REFERENCED_BY_RELATION YES)
        set(DOXYGEN_SOURCE_BROWSER YES)

        # TODO: add other doxygen-awesome extensions
        set(DOXYGEN_HTML_EXTRA_STYLESHEET ${CMAKE_CURRENT_SOURCE_DIR}/../../.doxygen/doxygen-awesome.css)

        doxygen_add_docs(doxygen_omni_web ${CMAKE_CURRENT_SOURCE_DIR})

        if(NOT TARGET doxygen)
                add_custom_target(doxygen)
        endif()

        add_dependencies(doxygen doxygen_omni_web)
endif()