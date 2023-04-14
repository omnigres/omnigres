include_guard(GLOBAL)

include(ExternalProject)

ExternalProject_Add(inja SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../misc/inja"
        PREFIX "${CMAKE_BINARY_DIR}/inja"
        BUILD_ALWAYS ON
        INSTALL_COMMAND "")