cmake_minimum_required(VERSION 3.25.1)
project(omni_codon)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13.1)
        message(FATAL_ERROR "GCC version must be at least 13.1 Detected version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif ()
endif ()

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

include(CompressedDependency)
set(_triple)
if (LINUX)
    string(APPEND _triple "linux-")
elseif (APPLE)
    string(APPEND _triple "darwin-")
endif ()
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64" AND APPLE)
    string(APPEND _triple "arm64")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64" AND LINUX)
    string(APPEND _triple "aarch64")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
    string(APPEND _triple "x86_64")
endif ()
prepare_compressed_dependency(codon FILE "${CMAKE_CURRENT_LIST_DIR}/codon-releases/codon-0.19.3-p1-${_triple}.tar.gz")
set(CODON_PATH "${codon_PATH}/codon-deploy-${_triple}")

find_library(LIBCODONC NAMES codonc PATHS "${CODON_PATH}/lib/codon" REQUIRED)
find_library(LIBCODONRT NAMES codonrt PATHS "${CODON_PATH}/lib/codon" REQUIRED)
find_library(LIBGCC_S NAMES gcc_s PATHS "${CODON_PATH}/lib/codon")
find_library(LIBOMP NAMES omp PATHS "${CODON_PATH}/lib/codon")
find_library(LIBQUADMATH NAMES quadmath PATHS "${CODON_PATH}/lib/codon")

add_postgresql_extension(
        omni_codon
        COMMENT "Exaloop Codon programming language"
        SOURCES omni_codon.cpp
        SCHEMA omni_codon
        TESTS_REQUIRE omni_schema omni_python
        RELOCATABLE false)

target_compile_features(omni_codon PUBLIC cxx_std_20)

target_include_directories(omni_codon PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/deps "${CODON_PATH}/include"
        "${CODON_PATH}/include/tser-1.0.0"
)

target_link_libraries(omni_codon PRIVATE  "${LIBCODONC}" "${LIBCODONRT}")
