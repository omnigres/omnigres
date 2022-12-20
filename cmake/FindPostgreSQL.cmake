# .rst: FindPostgreSQL
# --------------------
#
# Builds a PostgreSQL installation. As opposed to finding a system-wide installation, this module
# will download and build PostgreSQL with debug enabled.
#
# By default, it'll download the latest known version of PostgreSQL (at the time of last update)
# unless `PGVER` variable is set. `PGVER` can be either a major version like `15` which will be aliased
# to the latest known minor version, or a full version.
#
# This module defines the following variables
#
# ::
#
# PostgreSQL_LIBRARIES - the PostgreSQL libraries needed for linking
#
# PostgreSQL_INCLUDE_DIRS - include directories
#
# PostgreSQL_SERVER_INCLUDE_DIRS - include directories for server programming
#
# PostgreSQL_LIBRARY_DIRS  - link directories for PostgreSQL libraries
#
# PostgreSQL_EXTENSION_DIR  - the directory for extensions
#
# PostgreSQL_SHARED_LINK_OPTIONS  - options for shared libraries
#
# PostgreSQL_LINK_OPTIONS  - options for static libraries and executables
#
# PostgreSQL_VERSION_STRING - the version of PostgreSQL found (since CMake
# 2.8.8)
#
# ----------------------------------------------------------------------------
# History: This module is derived from the existing FindPostgreSQL.cmake and try
# to use most of the existing output variables of that module, but uses
# `pg_config` to extract the necessary information instead and add a macro for
# creating extensions. The use of `pg_config` is aligned with how the PGXS code
# distributed with PostgreSQL itself works.

# Copyright 2022 Omnigres Contributors
# Copyright 2020 Mats Kindahl
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

set(PGURL_15.1 https://ftp.postgresql.org/pub/source/v15.1/postgresql-15.1.tar.bz2)

# Use latest known version if PGVER is not set
if(NOT PGVER)
    set(PGVER 15)
endif()

# If the version is not known, try resolving the alias
set(PGVER_ALIAS_15 15.1)
if("${PGURL_${PGVER}}" STREQUAL "")
    set(PGVER_ALIAS "${PGVER_ALIAS_${PGVER}}")
    # If it still can't be resolved, fail
    if("${PGURL_${PGVER_ALIAS}}" EQUAL "")
        message(FATAL_ERROR "Can't resolve PostgreSQL version ${PGVER}")
    else()
        if (NOT _POSTGRESQL_ANNOUNCED)
            message(STATUS "Resolved PostgreSQL version alias ${PGVER} to ${PGVER_ALIAS}")
        endif()
    endif()
else()
    set(PGVER_ALIAS "${PGVER}")
endif()

# This is where we manage all PostgreSQL installations
set(PGDIR "${CMAKE_CURRENT_LIST_DIR}/../.pg")
# This is where we manage selected PostgreSQL version's installations
set(PGDIR_VERSION "${PGDIR}/${PGVER_ALIAS}")

if(NOT EXISTS "${PGDIR_VERSION}/build/bin/postgres")
    file(MAKE_DIRECTORY ${PGDIR})
    message(STATUS "Downloading PostgreSQL ${PGVER}")
    file(DOWNLOAD "${PGURL_${PGVER_ALIAS}}" "${PGDIR}/postgresql-${PGVER_ALIAS}.tar.bz2" SHOW_PROGRESS)
    message(STATUS "Extracting PostgreSQL ${PGVER}")
    file(ARCHIVE_EXTRACT INPUT "${PGDIR}/postgresql-${PGVER}.tar.bz2" DESTINATION ${PGDIR_VERSION})
    execute_process(
            COMMAND ./configure --enable-debug --prefix "${PGDIR_VERSION}/build"
            WORKING_DIRECTORY "${PGDIR_VERSION}/postgresql-${PGVER_ALIAS}")
    execute_process(
            COMMAND make install
            WORKING_DIRECTORY "${PGDIR_VERSION}/postgresql-${PGVER_ALIAS}")
endif()

set(PostgreSQL_ROOT "${PGDIR_VERSION}/build")

find_program(
        PG_CONFIG pg_config
        PATHS ${PostgreSQL_ROOT}
        REQUIRED
        NO_DEFAULT_PATH
        PATH_SUFFIXES bin)

if(NOT PG_CONFIG)
    message(FATAL_ERROR "Could not find pg_config")
else()
    set(PostgreSQL_FOUND TRUE)
endif()


if(PostgreSQL_FOUND)
    macro(PG_CONFIG VAR OPT)
        execute_process(
                COMMAND ${PG_CONFIG} ${OPT}
                OUTPUT_VARIABLE ${VAR}
                OUTPUT_STRIP_TRAILING_WHITESPACE)
    endmacro()

    pg_config(_pg_bindir --bindir)
    pg_config(_pg_includedir --includedir)
    pg_config(_pg_pkgincludedir --pkgincludedir)
    pg_config(_pg_sharedir --sharedir)
    pg_config(_pg_includedir_server --includedir-server)
    pg_config(_pg_libs --libs)
    pg_config(_pg_ldflags --ldflags)
    pg_config(_pg_ldflags_sl --ldflags_sl)
    pg_config(_pg_ldflags_ex --ldflags_ex)
    pg_config(_pg_pkglibdir --pkglibdir)
    pg_config(_pg_libdir --libdir)
    pg_config(_pg_version --version)

    separate_arguments(_pg_ldflags)
    separate_arguments(_pg_ldflags_sl)
    separate_arguments(_pg_ldflags_ex)

    set(_server_lib_dirs ${_pg_libdir} ${_pg_pkglibdir})
    set(_server_inc_dirs ${_pg_pkgincludedir} ${_pg_includedir_server})
    string(REPLACE ";" " " _shared_link_options
            "${_pg_ldflags};${_pg_ldflags_sl}")
    set(_link_options ${_pg_ldflags})
    if(_pg_ldflags_ex)
        list(APPEND _link_options ${_pg_ldflags_ex})
    endif()

    set(PostgreSQL_INCLUDE_DIRS
            "${_pg_includedir}"
            CACHE PATH
            "Top-level directory containing the PostgreSQL include directories."
            )
    set(PostgreSQL_EXTENSION_DIR
            "${_pg_sharedir}/extension"
            CACHE PATH "Directory containing extension SQL and control files")
    set(PostgreSQL_SERVER_INCLUDE_DIRS
            "${_server_inc_dirs}"
            CACHE PATH "PostgreSQL include directories for server include files.")
    set(PostgreSQL_LIBRARY_DIRS
            "${_pg_libdir}"
            CACHE PATH "library directory for PostgreSQL")
    set(PostgreSQL_LIBRARIES
            "${_pg_libs}"
            CACHE PATH "Libraries for PostgreSQL")
    set(PostgreSQL_SHARED_LINK_OPTIONS
            "${_shared_link_options}"
            CACHE STRING "PostgreSQL linker options for shared libraries.")
    set(PostgreSQL_LINK_OPTIONS
            "${_pg_ldflags},${_pg_ldflags_ex}"
            CACHE STRING "PostgreSQL linker options for executables.")
    set(PostgreSQL_SERVER_LIBRARY_DIRS
            "${_server_lib_dirs}"
            CACHE PATH "PostgreSQL server library directories.")
    set(PostgreSQL_VERSION_STRING
            "${_pg_version}"
            CACHE STRING "PostgreSQL version string")
    set(PostgreSQL_PACKAGE_LIBRARY_DIR
            "${_pg_pkglibdir}"
            CACHE STRING "PostgreSQL package library directory")

    find_program(
            PG_BINARY postgres
            PATHS ${PostgreSQL_ROOT_DIRECTORIES}
            HINTS ${_pg_bindir}
            PATH_SUFFIXES bin)

    if(NOT PG_BINARY)
        message(FATAL_ERROR "Could not find postgres binary")
    endif()


    find_program(PG_REGRESS pg_regress HINT
            ${PostgreSQL_PACKAGE_LIBRARY_DIR}/pgxs/src/test/regress)
    if(NOT PG_REGRESS)
        message(WARNING "Could not find pg_regress, tests not executed")
    endif()

    find_program(
            INITDB initdb
            PATHS ${PostgreSQL_ROOT_DIRECTORIES}
            HINTS ${_pg_bindir}
            PATH_SUFFIXES bin)

    if(NOT INITDB)
        message(WARNING "Could not find initdb, psql_${NAME} will not be available")
    endif()

    find_program(
            CREATEDB createdb
            PATHS ${PostgreSQL_ROOT_DIRECTORIES}
            HINTS ${_pg_bindir}
            PATH_SUFFIXES bin)

    if(NOT CREATEDB)
        message(WARNING "Could not find createdb, psql_${NAME} will not be available")
    endif()

    find_program(
            PSQL psql
            PATHS ${PostgreSQL_ROOT_DIRECTORIES}
            HINTS ${_pg_bindir}
            PATH_SUFFIXES bin)

    if(NOT PSQL)
        message(WARNING "Could not find psql, psql_${NAME} will not be available")
    endif()

    find_program(
            PG_CTL pg_ctl
            PATHS ${PostgreSQL_ROOT_DIRECTORIES}
            HINTS ${_pg_bindir}
            PATH_SUFFIXES bin)

    if(NOT PG_CTL)
        message(WARNING "Could not find pg_ctl, psql_${NAME} will not be available")
    endif()


    if (NOT _POSTGRESQL_ANNOUNCED)
        message(STATUS "Found postgres binary at ${PG_BINARY}")
        message(STATUS "PostgreSQL version ${PostgreSQL_VERSION_STRING} found")
        message(
                STATUS
                "PostgreSQL package library directory: ${PostgreSQL_PACKAGE_LIBRARY_DIR}")
        message(STATUS "PostgreSQL libraries: ${PostgreSQL_LIBRARIES}")
        message(STATUS "PostgreSQL extension directory: ${PostgreSQL_EXTENSION_DIR}")
        message(STATUS "PostgreSQL linker options: ${PostgreSQL_LINK_OPTIONS}")
        message(
            STATUS "PostgreSQL shared linker options: ${PostgreSQL_SHARED_LINK_OPTIONS}")
        set(_POSTGRESQL_ANNOUNCED ON CACHE INTERNAL "PostgreSQL was announced")
    endif()
endif()

# add_postgresql_extension(NAME ...)
#
# VERSION Version of the extension. Is used when generating the control file.
# Required.
#
# ENCODING Encoding for the control file. Optional.
#
# COMMENT Comment for the control file. Optional.
#
# SOURCES List of source files to compile for the extension.
#
# REQUIRES List of extensions that are required by this extension.
#
# SCRIPTS Script files.
#
# SCRIPT_TEMPLATES Template script files.
#
# REGRESS Regress tests.
#
# SCHEMA Extension schema.
#
# RELOCATABLE Is extension relocatable
#
#
# Defines the following targets:
#
# deploy      Deploys known all extensions to PostgreSQL
# deploy_NAME Deploys built extension to PostgreSQL (and adds it to `deploy` target)
# psql_NAME   Deploys built extensions ot PostgreSQL, starts it in a fresh database and connects with psql
#             (port assigned randomly)
#
# NAME_update_results Updates pg_regress test expectations to match results
# test_verbose_NAME Runs tests verbosely
function(add_postgresql_extension NAME)
    set(_optional)
    set(_single VERSION ENCODING SCHEMA RELOCATABLE)
    set(_multi SOURCES SCRIPTS SCRIPT_TEMPLATES REQUIRES REGRESS)
    cmake_parse_arguments(_ext "${_optional}" "${_single}" "${_multi}" ${ARGN})

    if(NOT _ext_VERSION)
        message(FATAL_ERROR "Extension version not set")
    endif()

    # Here we are assuming that there is at least one source file, which is
    # strictly speaking not necessary for an extension. If we do not have source
    # files, we need to create a custom target and attach properties to that. We
    # expect the user to be able to add target properties after creating the
    # extension.
    add_library(${NAME} MODULE ${_ext_SOURCES})

    target_compile_definitions(${NAME} PUBLIC "$<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Release>>:DEBUG>")

    set(_link_flags "${PostgreSQL_SHARED_LINK_OPTIONS}")
    foreach(_dir ${PostgreSQL_SERVER_LIBRARY_DIRS})
        set(_link_flags "${_link_flags} -L${_dir}")
    endforeach()

    # Collect and build script files to install
    set(_script_files)
    foreach(_script_file ${_ext_SCRIPTS})
        list(APPEND _script_files ${CMAKE_CURRENT_SOURCE_DIR}/${_script_file})
    endforeach()
    foreach(_template ${_ext_SCRIPT_TEMPLATES})
        string(REGEX REPLACE "\.in$" "" _script ${_template})
        configure_file(${_template} ${_script} @ONLY)
        list(APPEND _script_files ${CMAKE_CURRENT_BINARY_DIR}/${_script})
        message(
                STATUS "Building script file ${_script} from template file ${_template}")
    endforeach()

    if(APPLE)
        set(_link_flags "${_link_flags} -bundle_loader ${PG_BINARY}")
    endif()

    set_target_properties(
            ${NAME}
            PROPERTIES PREFIX ""
            LINK_FLAGS "${_link_flags}"
            POSITION_INDEPENDENT_CODE ON)

    target_include_directories(
            ${NAME}
            PRIVATE ${PostgreSQL_SERVER_INCLUDE_DIRS}
            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    # Generate control file at build time (which is when GENERATE evaluate the
    # contents). We do not know the target file name until then.
    set(_control_file "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.control")
    file(
            GENERATE
            OUTPUT ${_control_file}
            CONTENT
"default_version = '${_ext_VERSION}'
module_pathname = '$libdir/$<TARGET_FILE_NAME:${NAME}>'
$<$<NOT:$<BOOL:${_ext_COMMENT}>>:#>comment = '${_ext_COMMENT}'
$<$<NOT:$<BOOL:${_ext_ENCODING}>>:#>encoding = '${_ext_ENCODING}'
$<$<NOT:$<BOOL:${_ext_REQUIRES}>>:#>requires = '$<JOIN:${_ext_REQUIRES},$<COMMA>>'
$<$<NOT:$<BOOL:${_ext_SCHEMA}>>:#>schema = ${_ext_SCHEMA}
$<$<NOT:$<BOOL:${_ext_RELOCATABLE}>>:#>relocatable = ${_ext_RELOCATABLE}
")

    if(NOT TARGET deploy)
        add_custom_target(deploy)
    endif()
    add_custom_target(deploy_${NAME}
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.so ${PostgreSQL_PACKAGE_LIBRARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${_control_file} ${PostgreSQL_EXTENSION_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${_script_files} ${PostgreSQL_EXTENSION_DIR}
            )
    add_dependencies(deploy_${NAME} ${NAME})
    add_dependencies(deploy deploy_${NAME})

    if(_ext_REGRESS)
        foreach(_test ${_ext_REGRESS})
            set(_sql_file "${CMAKE_CURRENT_SOURCE_DIR}/sql/${_test}.sql")
            set(_out_file "${CMAKE_CURRENT_SOURCE_DIR}/expected/${_test}.out")
            if(NOT EXISTS "${_sql_file}")
                message(FATAL_ERROR "Test file '${_sql_file}' does not exist!")
            endif()
            if(NOT EXISTS "${_out_file}")
                file(WRITE "${_out_file}" )
                message(STATUS "Created empty file ${_out_file}")
            endif()
        endforeach()

        if(PG_REGRESS)
            list(JOIN _ext_REGRESS " " _ext_REGRESS_ARGS)
            file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_${PROJECT_NAME}
                    CONTENT "#! /usr/bin/env bash
# Pick random port
while true
do
    export PORT=$(( ((RANDOM<<15)|RANDOM) % 49152 + 10000 ))
    status=\"$(nc -z 127.0.0.1 $random_port < /dev/null &>/dev/null; echo $?)\"
    if [ \"${status}\" != \"0\" ]; then
        echo \"Using port $PORT\";
        break;
    fi
done
EXTENSION_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} \
${PG_REGRESS} --temp-instance=${CMAKE_BINARY_DIR}/tmp_check --inputdir=${CMAKE_CURRENT_SOURCE_DIR} \
--outputdir=${CMAKE_CURRENT_BINARY_DIR} --load-extension=${NAME} --host=127.0.0.1 --port=$PORT ${_ext_REGRESS_ARGS}
"
                    FILE_PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE
                    )
            add_test(
                    NAME ${NAME}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_${PROJECT_NAME}
                    )
        endif()

        add_custom_target(
                ${NAME}_update_results
                COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_BINARY_DIR}/results/*.out
                ${CMAKE_CURRENT_SOURCE_DIR}/expected)
        add_dependencies(${NAME}_update_results deploy_${NAME})
    endif()

    if(INITDB AND CREATEDB AND PSQL AND PG_CTL)
        file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/psql_${PROJECT_NAME}
            CONTENT "#! /usr/bin/env bash
# Pick random port
while true
do
    export PGPORT=$(( ((RANDOM<<15)|RANDOM) % 49152 + 10000 ))
    status=\"$(nc -z 127.0.0.1 $random_port < /dev/null &>/dev/null; echo $?)\"
    if [ \"${status}\" != \"0\" ]; then
        echo \"Using port $PGPORT\";
        break;
    fi
done
export EXTENSION_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
rm -rf \"${CMAKE_CURRENT_BINARY_DIR}/data/${NAME}\"
${INITDB} -D \"${CMAKE_CURRENT_BINARY_DIR}/data/${NAME}\" --no-clean --no-sync
export SOCKDIR=$(mktemp -d)
${PG_CTL} start -D \"${CMAKE_CURRENT_BINARY_DIR}/data/${NAME}\" -o \"-c port=$PGPORT\" -o -F -o -k -o \"$SOCKDIR\"
${CREATEDB} -h \"$SOCKDIR\" ${NAME}
${PSQL} -h \"$SOCKDIR\" ${NAME}
${PG_CTL} stop -D  \"${CMAKE_CURRENT_BINARY_DIR}/data/${NAME}\" -m smart
"
            FILE_PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE
        )
        add_custom_target(psql_${PROJECT_NAME}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/psql_${PROJECT_NAME})
        add_dependencies(psql_${PROJECT_NAME} deploy_${NAME})
    endif()
endfunction()

if(PG_REGRESS)
    # We add a custom target to get output when there is a failure.
    add_custom_target(
            test_verbose_${PROJECT_NAME} COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process
            --verbose --output-on-failure)
endif()