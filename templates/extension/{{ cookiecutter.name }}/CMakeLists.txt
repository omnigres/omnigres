cmake_minimum_required(VERSION 3.25.1)
project({{ cookiecutter.name }})

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        {{ cookiecutter.name }}
        COMMENT "{{ cookiecutter.description }}"
        SCHEMA {{ cookiecutter.schema }}
        RELOCATABLE false)
