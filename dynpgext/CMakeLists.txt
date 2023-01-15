cmake_minimum_required(VERSION 3.25.1)
project(dynpgext)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

find_package(PostgreSQL REQUIRED)
find_package(Doxygen)

add_library(dynpgext INTERFACE)
target_include_directories(dynpgext
    INTERFACE ${PostgreSQL_SERVER_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_BRIEF "Dynamic Postgres Extensions")

    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_FULL_SIDEBAR NO)

    set(DOXYGEN_OUTPUT_FOR_C YES)
    set(DOXYGEN_MARKDOWN_SUPPORT YES)
    set(DOXYGEN_AUTOLINK_SUPPORT YES)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE docs/index.md)

    # TODO: add other doxygen-awesome extensions
    set(DOXYGEN_HTML_EXTRA_STYLESHEET ${CMAKE_CURRENT_SOURCE_DIR}/../.doxygen/doxygen-awesome.css)

    doxygen_add_docs(doxygen_dynpgext ${CMAKE_CURRENT_SOURCE_DIR})

    if(NOT TARGET doxygen)
        add_custom_target(doxygen)
        add_dependencies(doxygen doxygen_dynpgext)
    endif()
endif()