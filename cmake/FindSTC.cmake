include(CPM)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/versions.cmake)
CPMAddPackage(NAME stc GIT_REPOSITORY https://github.com/stclib/STC GIT_TAG ${GIT_TAG_stc} VERSION ${VERSION_stc} OPTIONS "BUILD_TESTING OFF")