cmake_minimum_required(VERSION 3.25.1)
project(omni_regex)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

include(CPM)
include(CTest)

enable_testing()

find_package(PostgreSQL REQUIRED)

include(../../cmake/dependencies/versions.cmake)

CPMAddPackage(NAME pcre2 GITHUB_REPOSITORY "PCRE2Project/pcre2" GIT_TAG ${GIT_TAG_pcre2} VERSION ${VERSION_pcre2}
        OPTIONS "PCRE2_STATIC_PIC ON" "PCRE2_SUPPORT_JIT ON")

add_postgresql_extension(
        omni_regex
        SOURCES omni_regex.c pcre2.c
        RELOCATABLE true
)

target_link_libraries(omni_regex pcre2-8-static)