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

set_property(GLOBAL PROPERTY version_dir "${CMAKE_CURRENT_LIST_DIR}")

macro(get_version NAME VERSION_VAR)

    string(TOUPPER ${NAME} _name)

    if(DEFINED ${_name}_VERSION)
        set("${VERSION_VAR}" "${${_name}_VERSION}")
    elseif(DEFINED OMNIGRES_VERSION)
        set("${VERSION_VAR}" "${OMNIGRES_VERSION}")
    else()
        get_property(dir GLOBAL PROPERTY version_dir)
        string(STRIP "${VERSION_VAR}" VERSION_VAR)

        file(STRINGS ${dir}/../versions.txt lines)
        set(seen_extensions "")
        set(line_regex "^[a-z0-9_]+=[0-9]+\\.[0-9]+\\.[0-9]+")
        foreach(line ${lines})
            string(REGEX MATCH ${line_regex}  line_match ${line})
            if(NOT "${line_match}" STREQUAL ${line})
                message(FATAL_ERROR "\"${line}\" does not match with regex \"${line_regex}\" in ${dir}/../versions.txt")
            endif()
            string(FIND ${line} "=" line_name_length)
            string(SUBSTRING ${line} 0 ${line_name_length} line_name)
            if("${line_name}" IN_LIST seen_extensions)
                message(FATAL_ERROR "\"${line_name}\" found more than once in ${dir}/../versions.txt")
            endif()
            list(APPEND seen_extensions ${line_name})
            MATH(EXPR line_version_index "${line_name_length}+1")
            string(SUBSTRING ${line} ${line_version_index} -1 line_version)
            if("${line_name}" STREQUAL ${NAME})
                set("${VERSION_VAR}" "${line_version}")
            endif()
        endforeach()
        if(NOT "${NAME}" IN_LIST seen_extensions)
            set("${VERSION_VAR}" unreleased)
        endif()
    endif()

    unset(_name)

endmacro()