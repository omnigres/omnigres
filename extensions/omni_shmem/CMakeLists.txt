cmake_minimum_required(VERSION 3.25.1)
project(omni_shmem)

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
prepare_compressed_dependency(boost_1_88_0) # TODO: upgrade to 1.89

add_postgresql_extension(
        omni_shmem
        COMMENT "Shared Memory Management"
        SOURCES omni_shmem.cpp tests.cpp
        SCHEMA omni_shmem
        DEPENDS_ON omni libomni
        RELOCATABLE false)


target_compile_features(omni_shmem PUBLIC cxx_std_20)
target_link_libraries(omni_shmem libomni)


target_include_directories(omni_shmem PRIVATE
        ${boost_1_88_0_PATH}
        ${CMAKE_CURRENT_LIST_DIR}/deps
        ${CMAKE_CURRENT_LIST_DIR}/../../deps
        ${CMAKE_CURRENT_LIST_DIR}/..
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../../apex
        ${CMAKE_CURRENT_LIST_DIR}/../../deps/fmt/include)
