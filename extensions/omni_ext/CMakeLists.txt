cmake_minimum_required(VERSION 3.25.1)
project(omni_ext)

include(CPM)
include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)
find_package(STC REQUIRED)

function(check_symbols)
        set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
        check_symbol_exists(strverscmp "string.h" HAVE_STRVERSCMP)
        set(CMAKE_REQUIRED_LIBRARIES dl)
        check_symbol_exists(dladdr "dlfcn.h" HAVE_DLADDR)
endfunction()

check_symbols()

add_postgresql_extension(
        omni_ext
        VERSION 0.1
        SCHEMA omni_ext
        RELOCATABLE false
        SCRIPTS omni_ext--0.1.sql
        SOURCES omni_ext.c control_file.c init.c workers.c strverscmp.c
        SHARED_PRELOAD ON)

set_property(TARGET omni_ext PROPERTY C_STANDARD 11)

target_compile_definitions(omni_ext PUBLIC EXT_LIBRARY_NAME=\"$<TARGET_FILE_NAME:omni_ext>\")

if(HAVE_STRVERSCMP)
        target_compile_definitions(omni_ext PUBLIC HAVE_STRVERSCMP PUBLIC _GNU_SOURCE)
endif()

if(HAVE_DLADDR)
        target_compile_definitions(omni_ext PUBLIC HAVE_DLADDR PUBLIC _GNU_SOURCE)
endif()

target_link_libraries(omni_ext libpgaug dynpgext libgluepg_stc)

add_subdirectory(test)