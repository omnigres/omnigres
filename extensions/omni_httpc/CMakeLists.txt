cmake_minimum_required(VERSION 3.25.1)
project(omni_httpc)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)
include(OpenSSL)

include(CTest)
include(FindPkgConfig)

enable_testing()

find_package(PostgreSQL REQUIRED)
find_package(H2O REQUIRED)

pkg_check_modules(BROTLI_DEC libbrotlidec)
pkg_check_modules(BROTLI_ENC libbrotlienc)

add_postgresql_extension(
        omni_httpc
        SCHEMA omni_httpc
        RELOCATABLE false
        SOURCES omni_httpc.c ca-bundle.cc ssl.c
        DEPENDS_ON libpgaug omni_sql omni_types
        REQUIRES omni_http omni_types)

# Raw string literal in ca-bundle.cc
set_property(TARGET omni_httpc PROPERTY CXX_STANDARD 11)

target_link_libraries(omni_httpc libh2o-evloop libpgaug libomnisql libomnitypes ${OPENSSL_LIBRARIES})

target_compile_definitions(omni_httpc PRIVATE H2O_USE_LIBUV=0)

get_target_property(_h2o_deps libh2o INCLUDE_DIRECTORIES)
target_include_directories(omni_httpc PUBLIC $<TARGET_PROPERTY:libh2o,INCLUDE_DIRECTORIES>)

if(BROTLI_DEC_FOUND AND BROTLI_ENC_FOUND)
        target_link_directories(omni_httpc PUBLIC ${BROTLI_DEC_LIBRARY_DIRS} ${BROTLI_ENC_LIBRARY_DIRS})
endif()

find_package(Doxygen)

if(DOXYGEN_FOUND)
        set(DOXYGEN_PROJECT_BRIEF "omni_httpc")

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

        doxygen_add_docs(doxygen_omni_httpc ${CMAKE_CURRENT_SOURCE_DIR})

        if(NOT TARGET doxygen)
                add_custom_target(doxygen)
        endif()

        add_dependencies(doxygen doxygen_omni_httpc)
endif()