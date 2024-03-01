cmake_minimum_required(VERSION 3.25.1)
project(tools)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_test(
        NAME tools/check_extension_artifacts
        COMMAND ${Python3_EXECUTABLE} -m unittest test.py
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tools")

execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt)