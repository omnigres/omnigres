cmake_minimum_required(VERSION 3.25.1)
project(tools)

add_test(
        NAME tools/check_extension_artifacts
        COMMAND python -m unittest check_extension_artifacts.py
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tools")