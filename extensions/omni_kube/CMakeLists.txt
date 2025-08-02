cmake_minimum_required(VERSION 3.25.1)
project(omni_kube)

include(CTest)
include(CheckIncludeFile)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_kube
        VERSION 0.1.1
        COMMENT "Kubernetes (k8s) integration"
        SCHEMA omni_kube
        RELOCATABLE false
        REQUIRES omni_httpc omni_web omni_var pgcrypto)
