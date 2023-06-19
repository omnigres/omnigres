cmake_minimum_required(VERSION 3.25.1)
project(libgluepg_stc)

if(NOT DEFINED LIBGLUEPG_STC_ALREADY_INCLUDED)
    set(LIBGLUEPG_STC_ALREADY_INCLUDED TRUE CACHE INTERNAL "")

    include(CTest)

    list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

    enable_testing()

    find_package(PostgreSQL REQUIRED)
    find_package(STC REQUIRED)

    if(NOT TARGET libpgaug)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../libpgaug" "${CMAKE_CURRENT_BINARY_DIR}/libpgaug")
    endif()

    add_library(libgluepg_stc INTERFACE)
    target_include_directories(libgluepg_stc
            INTERFACE ${PostgreSQL_SERVER_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(libgluepg_stc INTERFACE stc libpgaug)

endif()