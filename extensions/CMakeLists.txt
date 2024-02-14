cmake_minimum_required(VERSION 3.25.1)
project(omnigres)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake)

include(Common)

enable_testing()

# Find extensions automatically so we don't have to add them manually
file(GLOB _extension_dir_items RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)
foreach(_extension_dir_item ${_extension_dir_items})
    file(REAL_PATH ${CMAKE_CURRENT_LIST_DIR}/${_extension_dir_item} _extension_dir_item_path)
    # must have <extension>/CMakeLists.txt
    if(IS_DIRECTORY ${_extension_dir_item_path} AND EXISTS ${_extension_dir_item_path}/CMakeLists.txt)
        add_subdirectory_once(${_extension_dir_item})
    endif()
endforeach()

execute_process(COMMAND python "${CMAKE_SOURCE_DIR}/tools/check_extension_artifacts.py" "${CMAKE_BINARY_DIR}/artifacts.txt" COMMAND_ERROR_IS_FATAL ANY)