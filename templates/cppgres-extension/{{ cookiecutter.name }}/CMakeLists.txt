cmake_minimum_required(VERSION 3.25.1)
project({{ cookiecutter.name }})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13.1)
        message(FATAL_ERROR "GCC version must be at least 13.1 Detected version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif ()
endif ()

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        {{ cookiecutter.name }}
        COMMENT "{{ cookiecutter.description }}"
        SOURCES {{ cookiecutter.main_source }}
        SCHEMA {{ cookiecutter.schema }}
        {% if cookiecutter.omni_extension %}
        DEPENDS_ON omni libomni
        {% endif %}
        RELOCATABLE false)


target_compile_features({{ cookiecutter.name }} PUBLIC cxx_std_20)
{%- if cookiecutter.omni_extension %}
target_link_libraries({{ cookiecutter.name }} libomni)
{% endif %}

target_include_directories({{ cookiecutter.name}} PRIVATE
${CMAKE_CURRENT_LIST_DIR}/deps
${CMAKE_CURRENT_LIST_DIR}/../../deps/fmt/include)
