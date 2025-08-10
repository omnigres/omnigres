cmake_minimum_required(VERSION 3.25.1)
project(omni_datasets)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_datasets
        COMMENT "Dataset provisioning"
        SCHEMA omni_datasets
        RELOCATABLE false)
