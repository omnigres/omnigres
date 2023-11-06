cmake_minimum_required(VERSION 3.25.1)
project(omnigres)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

enable_testing()

add_subdirectory(misc/inja)
add_subdirectory(pg_yregress)
add_subdirectory(dynpgext)
add_subdirectory(libpgaug)
add_subdirectory(libgluepg_curl)
add_subdirectory(libgluepg_yyjson)
add_subdirectory(libgluepg_stc)
add_subdirectory(extensions)
add_subdirectory(languages)

include(CPM)
CPMAddPackage(
    NAME Format.cmake
    VERSION 1.7.3
    GITHUB_REPOSITORY TheLartians/Format.cmake
    OPTIONS
    "FORMAT_SKIP_CMAKE YES"
)