include(CPM)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/versions.cmake)
CPMAddPackage(NAME metalang99 GIT_REPOSITORY https://github.com/Hirrolot/metalang99 VERSION ${VERSION_metalang})