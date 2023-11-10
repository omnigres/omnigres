cmake_minimum_required(VERSION 3.25.1)
project(omnigres)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

enable_testing()

include(Common)

add_subdirectory_once(misc/inja)
add_subdirectory_once(pg_yregress)
add_subdirectory_once(dynpgext)
add_subdirectory_once(libpgaug)
add_subdirectory_once(libgluepg_stc)
add_subdirectory_once(extensions)
add_subdirectory_once(languages)

include(CPM)
CPMAddPackage(
    NAME Format.cmake
    VERSION 1.7.3
    GITHUB_REPOSITORY TheLartians/Format.cmake
    OPTIONS
    "FORMAT_SKIP_CMAKE YES"
)