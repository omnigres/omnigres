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