cmake_minimum_required(VERSION 3.25.1)
project(omni_ext)


list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

include(CPM)
include(CTest)
include(CheckSymbolExists)

enable_testing()

find_package(PostgreSQL REQUIRED)
check_symbol_exists(dladdr "dlfcn.h" HAVE_DLADDR)

add_postgresql_extension(
        omni
        COMMENT "Advanced adapter for Postgres extensions"
        SCHEMA omni
        RELOCATABLE false
        SOURCES init.c omni.c hook_harness.c module.c extension.c workers.c utils.c dshash.c
        DEPENDS_ON libomni
        SHARED_PRELOAD ON)

target_link_libraries(omni libomni)
target_include_directories(omni PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

function(check_symbols)
    set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
    set(CMAKE_REQUIRED_LIBRARIES dl)
    check_symbol_exists(dladdr "dlfcn.h" HAVE_DLADDR)
endfunction()

check_symbols()

if(HAVE_DLADDR)
    target_compile_definitions(omni PUBLIC HAVE_DLADDR PUBLIC _GNU_SOURCE)
endif()

target_compile_definitions(omni PUBLIC EXT_LIBRARY_NAME=\"$<TARGET_FILE_NAME:omni>\")

if(NOT DEFINED OMNI_UNITY_BUILD)
    # By default, let's do a unity build to ensure we get all optimizations from it
    # that we can, considering that it may be in a hot path
    set(OMNI_UNITY_BUILD ON)
endif()

if(OMNI_UNITY_BUILD)
    # Make it a unity build so it can benefit from static and inline
    # specifiers and other optimizations, while separating code between
    # different files.
    set_property(TARGET omni PROPERTY UNITY_BUILD ON)
    target_compile_definitions(omni PUBLIC UNITY_BUILD)
endif()

add_subdirectory(test)