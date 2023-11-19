if(NOT TARGET check_git_commit_hash)
    find_package(Git REQUIRED)

    if(DEFINED OMNIGRES_VERSION)
        file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/git_commit CONTENT "${OMNIGRES_VERSION}")
    else()
        add_custom_command(
                OUTPUT ${CMAKE_BINARY_DIR}/git_commit
                COMMAND ${GIT_EXECUTABLE} rev-parse HEAD > ${CMAKE_BINARY_DIR}/git_commit
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif()

    add_custom_target(check_git_commit_hash ALL DEPENDS ${CMAKE_BINARY_DIR}/git_commit)
endif()

macro(get_version NAME VERSION_VAR)

    string(TOUPPER ${NAME} _name)

    if(DEFINED ${_name}_VERSION)
        set("${VERSION_VAR}" "${${_name}_VERSION}")
    elseif(DEFINED OMNIGRES_VERSION)
        set("${VERSION_VAR}" "${OMNIGRES_VERSION}")
    else()
        execute_process(COMMAND git describe  --tags --always --match "^v"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                OUTPUT_STRIP_TRAILING_WHITESPACE
                OUTPUT_VARIABLE _version)

        string(STRIP "${VERSION_VAR}" VERSION_VAR)

        set("${VERSION_VAR}" "${_version}")
    endif()

    unset(_name)

endmacro()