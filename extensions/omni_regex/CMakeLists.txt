cmake_minimum_required(VERSION 3.25.1)
project(omni_regex)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

include(CPM)
include(CTest)

enable_testing()

find_package(PostgreSQL REQUIRED)

include(../../cmake/dependencies/versions.cmake)

if (NOT DEFINED pcre2-8-static)
CPMAddPackage(NAME pcre2 SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../deps/pcre2 VERSION ${VERSION_pcre2}
        OPTIONS "PCRE2_STATIC_PIC ON" "PCRE2_SUPPORT_JIT ON")
endif ()

add_postgresql_extension(
        omni_regex
        VERSION 0.1.0
        COMMENT "PCRE-compatible regular expressions"
        SOURCES omni_regex.c pcre2.c
        RELOCATABLE true
)

target_link_libraries(omni_regex pcre2-8-static)