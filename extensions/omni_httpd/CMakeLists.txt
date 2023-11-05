cmake_minimum_required(VERSION 3.25.1)
project(omni_httpd)

include(CTest)
include(FindPkgConfig)
include(FindThreads)


list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)
include(OpenSSL)

enable_testing()

find_package(PostgreSQL REQUIRED)
find_package(H2O REQUIRED)
find_package(Metalang REQUIRED)
if(NOT Threads_FOUND)
        message(FATAL_ERROR "Threading library required (such as pthreads)")
endif()

pkg_check_modules(BROTLI_DEC libbrotlidec)
pkg_check_modules(BROTLI_ENC libbrotlienc)

add_postgresql_extension(
        omni_httpd
        SCHEMA omni_httpd
        RELOCATABLE false
        SOURCES omni_httpd.c master_worker.c http_worker.c event_loop.c fd.c cascading_query.c
        DEPENDS_ON omni_sql omni_ext dynpgext libpgaug libgluepg_stc
        REQUIRES omni_types omni_http)

set_property(TARGET omni_httpd PROPERTY C_STANDARD 11)

target_link_libraries(omni_httpd libh2o-evloop libpgaug libomnisql libomnitypes
        libgluepg_stc dynpgext metalang99
        Threads::Threads)

# Disable full macro expansion backtraces for Metalang99.
if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        target_compile_options(omni_httpd PRIVATE -fmacro-backtrace-limit=1)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(omni_httpd PRIVATE -ftrack-macro-expansion=0)
endif()

target_compile_definitions(omni_httpd PRIVATE H2O_USE_LIBUV=0)

get_target_property(_h2o_deps libh2o INCLUDE_DIRECTORIES)
target_include_directories(omni_httpd PUBLIC $<TARGET_PROPERTY:libh2o,INCLUDE_DIRECTORIES>)

if(BROTLI_DEC_FOUND AND BROTLI_ENC_FOUND)
        target_link_directories(omni_httpd PUBLIC ${BROTLI_DEC_LIBRARY_DIRS} ${BROTLI_ENC_LIBRARY_DIRS})
endif()

find_package(Doxygen)

if(DOXYGEN_FOUND)
        set(DOXYGEN_PROJECT_BRIEF "omni_httpd")

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

        doxygen_add_docs(doxygen_omni_httpd ${CMAKE_CURRENT_SOURCE_DIR})

        if(NOT TARGET doxygen)
                add_custom_target(doxygen)
        endif()

        add_dependencies(doxygen doxygen_omni_httpd)
endif()