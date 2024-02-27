function(find_logtalk LOGTALK_VAR SWIPL_VERSION)
    list(LENGTH ARGN ARGN_LENGTH)
    if(ARGN_LENGTH GREATER 0)
        list(GET ARGN 0 LOGTALK_VERSION)
    else()
    endif()
    if(NOT LOGTALK_VERSION)
        set(LOGTALK_VERSION 3.75.0)
    endif()

    if(NOT DEFINED LOGTALK_GIT_TAG)
        string(REPLACE "." "" _LOGTALK_VERSION_DOTLESS ${LOGTALK_VERSION})
        set(LOGTALK_GIT_TAG lgt${_LOGTALK_VERSION_DOTLESS}stable)
    endif()

    if(NOT DEFINED LOGTALK_${LOGTALK_VERSION})
        CPMAddPackage(NAME Logtalk_${LOGTALK_VERSION} GITHUB_REPOSITORY LogtalkDotOrg/logtalk3 VERSION ${LOGTALK_VERSION} GIT_TAG ${LOGTALK_GIT_TAG} DOWNLOAD_ONLY YES)

        if(Logtalk_${LOGTALK_VERSION}_ADDED)
            set(ENV{LOGTALKHOME} ${Logtalk_${LOGTALK_VERSION}_SOURCE_DIR})
            set(ENV{LOGTALKUSER} ${Logtalk_${LOGTALK_VERSION}_SOURCE_DIR})
            set(ENV{PATH} ${CMAKE_CURRENT_BINARY_DIR}/swipl_${SWIPL_VERSION}/bin:$ENV{PATH})
            add_custom_target(swilgt_${LOGTALK_VERSION}
                    COMMAND LOGTALKUSER=${Logtalk_${LOGTALK_VERSION}_SOURCE_DIR} LOGTALKHOME=${Logtalk_${LOGTALK_VERSION}_SOURCE_DIR} ${SWIPL_${SWIPL_VERSION}} -s "${Logtalk_${LOGTALK_VERSION}_SOURCE_DIR}/integration/logtalk_swi.pl"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        endif()
    endif()
    set(${LOGTALK_VAR} swilgt_${LOGTALK_VERSION} PARENT_SCOPE)
    set(${LOGTALK_VAR}_SOURCE_DIR ${Logtalk_${LOGTALK_VERSION}_SOURCE_DIR} PARENT_SCOPE)
    set(${LOGTALK_VAR}_VERSION ${LOGTALK_VERSION} PARENT_SCOPE)
endfunction()


function(logtalk_qlf LOGTALK_VAR TARGET)
    set(_optional SETTINGS_BUILD)
    set(_single NAME LOADER)
    set(_multi SOURCES)
    cmake_parse_arguments(_qlf "${_optional}" "${_single}" "${_multi}" ${ARGN})
    get_target_property(_src ${TARGET} SOURCE_DIR)
    get_target_property(_bin ${TARGET} BINARY_DIR)
    get_target_property(_srcs ${TARGET} SOURCES)
    set(_sources)
    foreach(_srcn ${_qlf_SOURCES})
        list(APPEND _sources ${_src}/${_srcn})
    endforeach()
    set(_settings ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/settings-build.lgt)
    set(_extra_srcs)
    if(_qlf_SETTINGS_BUILD)
        string(APPEND _settings "${_src}/${_qlf_SETTINGS_BUILD}")
        list(APPEND _extra_srcs "${_src}/${_qlf_SETTINGS_BUILD}")
    endif()
    list(GET _srcs 0 _first_src)
    target_sources(${TARGET} PRIVATE ${_bin}/app/application)
    get_filename_component(loader_dir ${_src}/${_qlf_LOADER} DIRECTORY ABSOLUTE)
    add_custom_command(
            DEPENDS ${_sources} ${_src}/${_qlf_LOADER} ${_extra_srcs}
            WORKING_DIRECTORY ${loader_dir}
            OUTPUT ${_bin}/app/application
            COMMAND LOGTALKUSER=${${LOGTALK_VAR}_SOURCE_DIR} LOGTALKHOME=${${LOGTALK_VAR}_SOURCE_DIR} PATH=${${LOGTALK_VAR}_SOURCE_DIR}/integration:$ENV{PATH}
            ${${LOGTALK_VAR}_SOURCE_DIR}/scripts/embedding/swipl/swipl_logtalk_qlf.sh
            -f save
            -d ${_bin}/app
            -s ${_settings}
            -l ${_src}/${_qlf_LOADER}
            -c -x
            # Trigger a rebuild for the target. Not pretty but does the job
            COMMAND ${CMAKE_COMMAND} -E touch ${_src}/${_first_src})

    add_custom_command(TARGET ${TARGET} POST_BUILD
            DEPENDS ${_bin}/app/application
            COMMAND cat $<TARGET_FILE:${TARGET}> ${_bin}/app/application > ${_bin}/_temp
            COMMAND mv ${_bin}/_temp $<TARGET_FILE:${TARGET}>
            COMMAND chmod +x $<TARGET_FILE:${TARGET}>)

endfunction()