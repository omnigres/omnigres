include(CPM)
include(CheckCSourceCompiles)

set(_h2o_options)
list(APPEND _h2o_options "WITH_MRUBY OFF")
list(APPEND _h2o_options "DISABLE_LIBUV ON")
# This is to make h2o find WSLAY
list(APPEND _h2o_options "CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}")

find_package(WSLAY REQUIRED)

include(${CMAKE_CURRENT_LIST_DIR}/dependencies/versions.cmake)

cmake_policy(SET CMP0042 NEW)
CPMAddPackage(NAME h2o SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../deps/h2o VERSION ${VERSION_h2o} OPTIONS "${_h2o_options}")
set_property(TARGET libh2o-evloop PROPERTY POSITION_INDEPENDENT_CODE ON)

add_dependencies(libh2o wslay)
add_dependencies(libh2o-evloop wslay)