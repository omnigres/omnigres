cmake_minimum_required(VERSION 3.25.1)
project(omni_prolog)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

include(CPM)

enable_testing()

find_package(PostgreSQL REQUIRED)

if(NOT DEFINED SWIPL)

    CPMAddPackage(NAME swiprolog GITHUB_REPOSITORY SWI-Prolog/swipl-devel VERSION 9.3.18 GIT_TAG V9.3.18
            DOWNLOAD_ONLY YES GIT_SUBMODULES)

    if(swiprolog_ADDED)
        execute_process(COMMAND cmake -S ${swiprolog_SOURCE_DIR} -DMULTI_THREADED=OFF -DSWIPL_STATIC_LIB=ON
                -DSWIPL_PACKAGE_LIST=http;clib -DUSE_GMP=OFF -DUSE_SIGNALS=OFF
                -DSWIPL_PACKAGES_PYTHON=OFF
                -DSWIPL_STATIC_LIB=ON -DBUILD_TESTING=OFF -DINSTALL_TESTS=OFF
                -DINSTALL_DOCUMENTATION=OFF -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/swipl
                -B ${swiprolog_BINARY_DIR}
                RESULT_VARIABLE rc)

        if(NOT rc EQUAL 0)
            message(FATAL_ERROR "Failed configuring swipl")
        endif()

        execute_process(COMMAND cmake --build ${swiprolog_BINARY_DIR} --parallel --target install
                RESULT_VARIABLE rc)
        if(NOT rc EQUAL 0)
            message(FATAL_ERROR "Failed building swipl")
        endif()

        set(SWIPL ${CMAKE_CURRENT_BINARY_DIR}/swipl/bin/swipl)
    else()
        find_program(SWIPL swipl REQUIRED)
    endif()
endif()

execute_process(COMMAND ${SWIPL} --dump-runtime-variables OUTPUT_VARIABLE swipl_runtime_variables)
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

    set(${name} "${value}" CACHE INTERNAL "Swipl setting")
endforeach()

find_library(LIBSWIPL_STATIC NAMES swipl_static PATHS "${PLLIBDIR}")
find_library(LIBSWIPL NAMES swipl.9 PATHS "${PLLIBDIR}")

if(LIBSWIPL_STATIC)
    set(STATIC TRUE)
    set(libswipl swipl_static)
else()
    set(libswipl swipl)
endif()

add_postgresql_extension(
        omni_prolog
        SCHEMA omni_prolog
        RELOCATABLE false
        SOURCES omni_prolog.c install.c)

target_compile_definitions(omni_prolog PRIVATE __SWI_PROLOG __SWI_EMBEDDED_)
target_include_directories(omni_prolog PRIVATE "${PLBASE}/include")
target_link_libraries(omni_prolog ${libswipl})
target_link_directories(omni_prolog PRIVATE "${PLLIBDIR}")


set_target_properties(omni_prolog PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
if(APPLE)
    set_target_properties(omni_prolog PROPERTIES INSTALL_RPATH "@loader_path")
else()
    set_target_properties(omni_prolog PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()

target_link_libraries(omni_prolog libpgaug)

if(STATIC)
    if(APPLE)
        find_library(CoreFoundation NAMES CoreFoundation)
        target_link_libraries(omni_prolog ${CoreFoundation})
    endif()

    find_library(libncurses NAMES ncurses REQUIRED)
    find_library(libz NAMES z REQUIRED)
    find_library(libm NAMES m REQUIRED)

    target_link_libraries(omni_prolog ${libncurses} ${libz} ${libm})
endif()

# The following stub library is used by omni_prolog
add_library(omni_prolog_stub SHARED stub.c install.c)
target_link_libraries(omni_prolog_stub swipl) # always use the shared library
target_link_directories(omni_prolog_stub PRIVATE "${PLLIBDIR}")
target_compile_definitions(omni_prolog_stub PRIVATE __SWI_PROLOG __SWI_EMBEDDED_)
target_include_directories(omni_prolog_stub PRIVATE "${PLBASE}/include")
set_target_properties(omni_prolog_stub PROPERTIES PREFIX "" SUFFIX ".so")

add_custom_command(OUTPUT prolog_saved
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/omni_prolog.pl omni_prolog_stub
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${SWIPL} -f none -F none -g true -t "consult('${CMAKE_CURRENT_SOURCE_DIR}/omni_prolog.pl'),qsave_program('prolog_saved',[goal(true),toplevel(prolog),init_file(none),stand_alone(true),undefined(error),verbose(true)])"
        # Trigger a rebuild for the extension. Not pretty but does the job
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_SOURCE_DIR}/omni_prolog.c
        VERBATIM)

add_custom_target(prolog_saved_target ALL DEPENDS prolog_saved)

add_dependencies(omni_prolog prolog_saved_target)

add_custom_command(TARGET omni_prolog POST_BUILD
        DEPENDS prolog_saved_target
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND cat $<TARGET_FILE:omni_prolog> ${CMAKE_CURRENT_BINARY_DIR}/prolog_saved > ${CMAKE_CURRENT_BINARY_DIR}/omni_prolog_temp
        COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/omni_prolog_temp $<TARGET_FILE:omni_prolog>
        VERBATIM)
