function(find_swipl SWIPL_VAR)
    list(LENGTH ARGN ARGN_LENGTH)
    if(ARGN_LENGTH GREATER 0)
        list(GET ARGN 0 SWIPL_VERSION)
    else()
    endif()
    if(NOT SWIPL_VERSION)
        set(SWIPL_VERSION 9.3.1)
    endif()
    if(NOT DEFINED SWIPL_${SWIPL_VERSION})
        CPMAddPackage(NAME swiprolog_${SWIPL_VERSION} GITHUB_REPOSITORY SWI-Prolog/swipl-devel VERSION ${SWIPL_VERSION} GIT_TAG V${SWIPL_VERSION}
                DOWNLOAD_ONLY YES
                GIT_SUBMODULES packages/http packages/ssl packages/clib packages/sgml packages/zlib packages/readline packages/yaml
                packages/cpp packages/utf8proc)

        if(swiprolog_${SWIPL_VERSION}_ADDED)
            execute_process(COMMAND cmake -S ${swiprolog_${SWIPL_VERSION}_SOURCE_DIR} -DSWIPL_STATIC_LIB=ON
                    -DSWIPL_PACKAGE_LIST=utf8proc\;http\;clib\;readline\;cpp\;yaml
                    -DSWIPL_STATIC_LIB=ON -DBUILD_TESTING=OFF -DINSTALL_TESTS=OFF
                    -DINSTALL_DOCUMENTATION=OFF -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/swipl_${SWIPL_VERSION}
                    -B ${swiprolog_${SWIPL_VERSION}_BINARY_DIR}
                    RESULT_VARIABLE rc)

            if(NOT rc EQUAL 0)
                message(FATAL_ERROR "Failed configuring swipl")
            endif()

            execute_process(COMMAND cmake --build ${swiprolog_${SWIPL_VERSION}_BINARY_DIR} --parallel --target install
                    RESULT_VARIABLE rc)
            if(NOT rc EQUAL 0)
                message(FATAL_ERROR "Failed building swipl")
            endif()

            set(SWIPL_${SWIPL_VERSION} ${CMAKE_CURRENT_BINARY_DIR}/swipl_${SWIPL_VERSION}/bin/swipl CACHE STRING "swipl")
        else()
            find_program(SWIPL_${SWIPL_VERSION} swipl REQUIRED PATHS ${CMAKE_CURRENT_BINARY_DIR}/swipl_${SWIPL_VERSION}/bin NO_DEFAULT_PATH)
        endif()
    endif()

    set(${SWIPL_VAR} ${SWIPL_${SWIPL_VERSION}} PARENT_SCOPE)
    set(${SWIPL_VAR}_VERSION ${SWIPL_VERSION} PARENT_SCOPE)

    execute_process(COMMAND ${SWIPL_${SWIPL_VERSION}} --dump-runtime-variables OUTPUT_VARIABLE swipl_runtime_variables)
    foreach(line ${swipl_runtime_variables})
        string(STRIP ${line} line)
        if(line STREQUAL "")
            continue()
        endif()
        # Find the positions of '=' and the second '\"'
        string(FIND "${line}" "=" pos_equal)
        string(FIND "${line}" "\"" pos_quote REVERSE)

        # Extract the variable name and value
        math(EXPR length_name "${pos_equal}")
        math(EXPR start_value "${pos_equal} + 2")
        math(EXPR length_value "${pos_quote} - ${start_value}")

        string(SUBSTRING "${line}" 0 ${length_name} name)
        string(SUBSTRING "${line}" ${start_value} ${length_value} value)

        set(${SWIPL_VAR}_${name} "${value}" CACHE INTERNAL "Swipl setting")
    endforeach()
endfunction()

function(target_link_swipl TARGET SWIPL)
    # as long as we don't need foreign libraries, we're fine, # but the moment we do,
    # these static builds don't work.
    #find_library((${SWIPL_VAR}_LIBSWIPL_STATIC NAMES swipl_static PATHS "${${SWIPL_VAR}_PLLIBDIR}")
    #    unset(LIBSWIPL_STATIC)
    find_library(LIBSWIPL NAMES swipl PATHS "${${SWIPL}_PLLIBDIR}" NO_DEFAULT_PATH REQUIRED)

    if(LIBSWIPL_STATIC)
        set(libswipl swipl_static)
    else()
        set(libswipl swipl)
    endif()

    target_link_libraries(${TARGET} PRIVATE ${libswipl})
    target_compile_definitions(${TARGET} PRIVATE __SWI_PROLOG __SWI_EMBEDDED_)
    target_include_directories(${TARGET} PRIVATE "${${SWIPL}_PLBASE}/include")
    target_link_directories(${TARGET} PRIVATE "${${SWIPL}_PLLIBDIR}")

    if(APPLE)
        set_target_properties(${TARGET} PROPERTIES INSTALL_RPATH "@loader_path")
    else()
        set_target_properties(${TARGET} PROPERTIES INSTALL_RPATH "$ORIGIN")
    endif()

    if(STATIC)
        if(APPLE)
            find_library(CoreFoundation NAMES CoreFoundation)
            target_link_libraries(${TARGET} PRIVATE ${CoreFoundation})
            set_target_properties(${TARGET} PROPERTIES ENABLE_EXPORTS ON)
        endif()

        find_library(libncurses NAMES ncurses REQUIRED)
        find_library(libz NAMES z REQUIRED)
        find_library(libm NAMES m REQUIRED)

        target_link_libraries(${TARGET} PRIVATE ${libncurses} ${libz} ${libm})
    endif()

    if(NOT STATIC)
        if(IS_SYMLINK ${LIBSWIPL})
            file(READ_SYMLINK ${LIBSWIPL} LIBSWIPL_)
            get_filename_component(LIBSWIPL_ ${LIBSWIPL_} NAME)
        else()
            get_filename_component(LIBSWIPL_ ${LIBSWIPL} NAME)
        endif()
        add_custom_target(copy-${LIBSWIPL_} BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${LIBSWIPL_}
                COMMAND ${CMAKE_COMMAND} -E copy ${LIBSWIPL} ${CMAKE_CURRENT_BINARY_DIR}/${LIBSWIPL_}
                DEPENDS ${LIBSWIPL})
        add_dependencies(${TARGET} copy-${LIBSWIPL_})
    endif()

endfunction()