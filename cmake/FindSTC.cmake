include(CPM)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/versions.cmake)
CPMAddPackage(NAME stc SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../deps/STC VERSION ${VERSION_stc} OPTIONS "BUILD_TESTING OFF")