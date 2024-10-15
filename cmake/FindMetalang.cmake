include(CPM)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/versions.cmake)
CPMAddPackage(NAME metalang99 SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../deps/metalang99 VERSION ${VERSION_metalang})