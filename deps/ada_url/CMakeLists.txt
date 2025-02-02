cmake_minimum_required(VERSION 3.25.1)
project(ada_url)

include(../../cmake/dependencies/versions.cmake)

if (NOT DEFINED pcre2-8-static)
    CPMAddPackage(NAME pcre2 SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../pcre2 VERSION ${VERSION_pcre2}
            OPTIONS "PCRE2_STATIC_PIC ON" "PCRE2_SUPPORT_JIT ON")
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12.2)
        message(FATAL_ERROR "GCC version must be at least 12.2 Detected version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif ()
endif ()

add_library(ada_url STATIC ada.cpp pcre2.cpp)
set_property(TARGET ada_url PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(ada_url PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ada_url PUBLIC cxx_std_20)

# When need to check against a vetted (but unsafe) regex provider

if (DEFINED ADA_USE_UNSAFE_STD_REGEX_PROVIDER AND ADA_USE_UNSAFE_STD_REGEX_PROVIDER)
    target_compile_definitions(ada_url PRIVATE ADA_USE_UNSAFE_STD_REGEX_PROVIDER)
endif ()

target_link_libraries(ada_url PUBLIC pcre2-8-static)
