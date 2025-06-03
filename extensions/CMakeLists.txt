cmake_minimum_required(VERSION 3.25.1)
project(omnigres)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake)

include(Common)

enable_testing()

# Find extensions automatically so we don't have to add them manually
file(GLOB _extension_dir_items RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)

# Unless specified, build `OMNIGRES_INCLUDE` from what we can find,
# excluding those in `OMNIGRES_EXCLUDE`
if(NOT OMNIGRES_INCLUDE)
    foreach(_extension_dir_item ${_extension_dir_items})
        file(REAL_PATH ${CMAKE_CURRENT_LIST_DIR}/${_extension_dir_item} _extension_dir_item_path)
        # must have <extension>/CMakeLists.txt
        if(IS_DIRECTORY ${_extension_dir_item_path} AND EXISTS ${_extension_dir_item_path}/CMakeLists.txt AND NOT "${_extension_dir_item}" IN_LIST OMNIGRES_EXCLUDE)
            list(APPEND OMNIGRES_INCLUDE "${_extension_dir_item}")
        else ()
            message(STATUS "Ignoring ${_extension_dir_item}")
        endif ()
    endforeach ()
endif()

foreach(_extension_dir_item ${OMNIGRES_INCLUDE})
    add_subdirectory_once(${_extension_dir_item})
endforeach()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

execute_process(COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/check_extension_artifacts.py" "${CMAKE_BINARY_DIR}/artifacts.txt"
        COMMAND_ERROR_IS_FATAL ANY)
