cmake_minimum_required(VERSION 3.25.1)
project(omni_kube)

include(CTest)
include(CheckIncludeFile)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_kube
        COMMENT "Kubernetes (k8s) integration"
        SCHEMA omni_kube
        RELOCATABLE false
        SOURCES omni_kube.c
        REQUIRES omni_httpc omni_web omni_var omni_yaml)
