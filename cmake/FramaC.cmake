find_program(FRAMAC frama-c)

if(FRAMAC)
    find_package(PostgreSQL)

    # We're doing a quick hack to ensure FramaC can work with PostgreSQL for now
    # Frama C doesn't support __int128 so we're working around it.
    # See https://git.frama-c.com/pub/frama-c/-/issues/2641
    #
    # The idea is that we undefine PG_INT128_TYPE if __FRAMAC_ is defined:
    file(READ ${PostgreSQL_ROOT}/include/postgresql/server/pg_config_manual.h _pg_config_manual)

    if(NOT _pg_config_manual MATCHES __FRAMAC__)
        file(APPEND ${PostgreSQL_ROOT}/include/postgresql/server/pg_config_manual.h [=[
 #ifdef __FRAMAC__
 #undef PG_INT128_TYPE
 #endif
 ]=])
    endif()

    execute_process(COMMAND frama-c --version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        OUTPUT_VARIABLE FRAMAC_VERSION)
    message(STATUS "Found Frama-C ${FRAMAC_VERSION}")

    if(NOT ${FRAMAC_VERSION} STREQUAL "26.0 (Iron)")
        message(STATUS "This version of Frama-C is unknown, disabling")
        unset(FRAMAC)
        unset(FRAMAC_VERSION)
    endif()
endif()

function(add_framac)
    if(FRAMAC)
        cmake_parse_arguments(FRAMAC "" "TARGET" "CFLAGS;FUNCTIONS" ${ARGN})

        if(NOT TARGET framac)
            add_custom_target(framac)
        endif()

        foreach(_cflags ${FRAMAC_CFLAGS})
            string(APPEND cpp_extra_args ${_cflags})
        endforeach()

        get_target_property(_targets ${FRAMAC_TARGET} LINK_LIBRARIES)
        list(APPEND _targets ${FRAMAC_TARGET})

        foreach(target ${_targets})
            get_target_property(_include_directories ${target} INCLUDE_DIRECTORIES)
            get_target_property(_interface_include_directories ${target} INTERFACE_INCLUDE_DIRECTORIES)

            foreach(_include_directory ${_include_directories})
                string(APPEND cpp_extra_args " -I ${_include_directory} ")
            endforeach()

            foreach(_include_directory ${_interface_include_directories})
                string(APPEND cpp_extra_args " -I ${_include_directory} ")
            endforeach()
        endforeach()

        get_target_property(_sources ${FRAMAC_TARGET} SOURCES)
        get_target_property(_source_dir ${FRAMAC_TARGET} SOURCE_DIR)
        set(_source_files)

        foreach(_source ${_sources})
            list(APPEND _source_files "${_source_dir}/${_source}")
        endforeach()

        set(_functions)

        foreach(function ${FRAMAC_FUNCTIONS})
            string(APPEND _functions ${function})
        endforeach()

        add_custom_target(framac_${FRAMAC_TARGET}
            COMMAND ${FRAMAC} -wp -wp-rte -wp-literals -kernel-msg-key pp -cpp-extra-args=${cpp_extra_args} -wp-fct ${_functions} ${_source_files} -then -report
        )
        add_dependencies(framac framac_${FRAMAC_TARGET})
    endif()
endfunction()
