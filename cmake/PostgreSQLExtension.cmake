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

# add_postgresql_extension(NAME ...)
#
# ENCODING Encoding for the control file. Optional.
#
# COMMENT Comment for the control file. Optional.
#
# SOURCES List of source files to compile for the extension. Optional.
#
# REQUIRES List of extensions that are required by this extension. Optional.
#
# TESTS_REQUIRE List of extensions that are required by tests. Optional.
#
# TESTS ON by default, if OFF, don't try finding tests for this extension
#
# REGRESS Regress tests.
#
# SCHEMA Extension schema.
#
# RELOCATABLE Is extension relocatable
#
# SUPERUSER Only superuser can create this extension
#
# SHARED_PRELOAD Is this a shared preload extension
#
# PRIVATE If true, this extension will not be automatically packaged
#
# DEPENDS_ON List of Omnigres components this extension depends
#
# VERSION Version of the extension. Is used when generating the control file.
# Optional, typically used only in scenarios when versions must be fixed.
#
# By default, version is picked from git revision, unless overriden by the following
# CMake variables:
#
# * OMNIGRES_VERSION: sets a version for all extensions
# * <EXTENSION_NAME_CAPITALIZED>_VERSION: sets a version for a particular extension
#
# TARGET Custom target name (by default, `NAME`)
#
# NO_DEFAULT_CONTROL Do not produce a default control file (can be useful alongside with `TARGET`)
#
# UPGRADE_SCRIPTS In certain cases, custom upgrade scripts need to be supplied.
#                 Currently, their vesioning is hardcoded and can't be based on extension's version
#                 (but this can change [TODO])
#
# Defines the following targets:
#
# psql_NAME   Start extension it in a fresh database and connects with psql
# (port assigned randomly unless specified using PGPORT environment variable)
# Note:
#
# If extension root directory contains `.psqlrc`, it will be executed in the begining
# of the `psql_NAME` session and `:CMAKE_BINARY_DIR` psql variable will be set to the current
# build directory (`CMAKE_BINARY_DIR`)
#
# prepare and prepare_NAME: prepares all the control/migration files
#
# NAME_update_results Updates pg_regress test expectations to match results
# test_verbose_NAME Runs tests verbosely
find_program(PGCLI pgcli)
find_program(NETCAT nc REQUIRED)

include(Version)
include(Common)

function(find_pg_yregress_tests dir)
    # Don't add the same tests more than once
    get_property(is_called GLOBAL PROPERTY "find_pg_yregress_tests:${dir}" SET)
    if(NOT is_called)
        get_filename_component(_source_dir "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
        if(NOT TARGET pg_yregress)
            add_subdirectory("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../pg_yregress" "${CMAKE_CURRENT_BINARY_DIR}/pg_yregress")
        endif()
        file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_ext_TARGET}_FindRegressTests.cmake"
                CONTENT "
file(GLOB_RECURSE files RELATIVE ${dir} LIST_DIRECTORIES false ${dir}/*.yml ${dir}/*.yaml)
list(SORT files)
foreach(file \${files})
    add_test(\"${_ext_TARGET}/\${file}\" ${CMAKE_BINARY_DIR}/script_${_ext_TARGET} ${_ext_dir} \"$<TARGET_FILE:pg_yregress>\" \"${dir}/\${file}\")
    set_tests_properties(\"${_ext_TARGET}/\${file}\" PROPERTIES
    WORKING_DIRECTORY \"${CMAKE_CURRENT_BINARY_DIR}\"
    ENVIRONMENT \"PGCONFIG=${PG_CONFIG};PGSHAREDIR=${_share_dir};OMNI_SO=$<$<TARGET_EXISTS:omni>:$<TARGET_FILE:omni>>;EXTENSION_FILE=$<$<STREQUAL:$<TARGET_PROPERTY:${NAME},TYPE>,MODULE_LIBRARY>:$<TARGET_FILE:${NAME}>>\")
endforeach()
")
        get_directory_property(current_includes TEST_INCLUDE_FILES)
        set_directory_properties(PROPERTIES TEST_INCLUDE_FILES "${current_includes};${CMAKE_CURRENT_BINARY_DIR}/${NAME}_FindRegressTests.cmake")
        set_property(GLOBAL PROPERTY "find_pg_yregress_tests:${dir}" "CALLED")
    endif()
endfunction()

function(add_postgresql_extension NAME)
    set(_optional SHARED_PRELOAD PRIVATE UNVERSIONED_SO NO_DEFAULT_CONTROL)
    set(_single VERSION ENCODING SCHEMA RELOCATABLE TESTS SUPERUSER TARGET COMMENT)
    set(_multi SOURCES REQUIRES TESTS_REQUIRE REGRESS DEPENDS_ON UPGRADE_SCRIPTS)
    cmake_parse_arguments(_ext "${_optional}" "${_single}" "${_multi}" ${ARGN})

    if(NOT DEFINED _ext_TARGET)
        set(_ext_TARGET ${NAME})
    endif()

    if(NOT DEFINED _ext_VERSION)
        get_version(${NAME} _ext_VERSION)
    endif()

    if(NOT _ext_VERSION)
        message(FATAL_ERROR "Extension version not set")
    endif()

    if(NOT _ext_SOURCES)
        add_custom_target(${_ext_TARGET})
    else()
        add_library(${_ext_TARGET} MODULE ${_ext_SOURCES})
    endif()

    if(NOT DEFINED _ext_SUPERUSER)
        set(_ext_SUPERUSER true)
    endif ()

    # inja ia a default target dependency for all extensions
    if(NOT TARGET inja)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../misc/inja" "${CMAKE_CURRENT_BINARY_DIR}/inja")
    endif()

    # omni is required by all other extensions.
    # When we build individual extensions separately, this condition will pass
    if(NOT TARGET omni AND NOT NAME STREQUAL "omni")
       add_subdirectory_once("${CMAKE_CURRENT_SOURCE_DIR}/../../omni" "${CMAKE_CURRENT_BINARY_DIR}/../../omni")
       add_subdirectory_once("${CMAKE_CURRENT_SOURCE_DIR}/../../extensions/omni" "${CMAKE_CURRENT_BINARY_DIR}/../../extensions/omni")
    endif()

    foreach(requirement ${_ext_REQUIRES})
        if(NOT TARGET ${requirement})
            if(EXISTS "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../${requirement}")
                add_subdirectory_once("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../${requirement}" "${CMAKE_BINARY_DIR}/${requirement}")
            elseif(EXISTS "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../extensions/${requirement}")
                add_subdirectory_once("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../extensions/${requirement}" "${CMAKE_BINARY_DIR}/extensions/${requirement}")
            elseif(EXISTS "${PostgreSQL_EXTENSION_DIR}/${requirement}.control")
                message(STATUS "Found matching installed extension")
            else()
                message(FATAL_ERROR "Can't find extension ${requirement}")
            endif()
        endif()
    endforeach()

    foreach(dependency ${_ext_DEPENDS_ON})

        if(NOT TARGET ${dependency})
            if(${dependency} STREQUAL libomni)
                add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../omni" "${CMAKE_CURRENT_BINARY_DIR}/libomni")
            else()
                if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../${dependency}")
                    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../${dependency}" "${CMAKE_CURRENT_BINARY_DIR}/${dependency}")
                elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../${dependency}")
                    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../${dependency}" "${CMAKE_CURRENT_BINARY_DIR}/${dependency}")
                elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../deps/${dependency}")
                    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../deps/${dependency}" "${CMAKE_CURRENT_BINARY_DIR}/${dependency}")
                else()
                    message(FATAL_ERROR "Can't find dependency ${dependency}")
                endif()
            endif()
        endif()
    endforeach()

    add_dependencies(${_ext_TARGET} inja)

    if(_ext_SOURCES)
        target_compile_definitions(${_ext_TARGET} PUBLIC "$<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Release>>:DEBUG>")
        target_compile_definitions(${_ext_TARGET} PUBLIC "$<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Release>>:USE_ASSERT_CHECKING>")
        target_compile_definitions(${_ext_TARGET} PUBLIC "EXT_VERSION=\"${_ext_VERSION}\"")
        target_compile_definitions(${_ext_TARGET} PUBLIC "EXT_SCHEMA=\"${_ext_SCHEMA}\"")
    endif()

    set(_link_flags "${PostgreSQL_SHARED_LINK_OPTIONS}")

    foreach(_dir ${PostgreSQL_SERVER_LIBRARY_DIRS})
        set(_link_flags "${_link_flags} -L${_dir}")
    endforeach()

    set(_share_dir "${CMAKE_BINARY_DIR}/pg-share")
    if(_pg_sharedir)
        file(COPY "${_pg_sharedir}/" DESTINATION "${_share_dir}" FOLLOW_SYMLINK_CHAIN NO_SOURCE_PERMISSIONS)
    endif()
    set(_ext_dir "${_share_dir}/extension")
    file(MAKE_DIRECTORY ${_ext_dir})

    if(APPLE)
        set(_link_flags "${_link_flags} -bundle -bundle_loader ${PG_BINARY}")
    endif()

    set(_suffix "--${_ext_VERSION}.so")
    if(_ext_UNVERSIONED_SO)
        set(_suffix ".so")
    endif()

    if(_ext_SOURCES)
        set_target_properties(
                ${_ext_TARGET}
                PROPERTIES
                OUTPUT_NAME "${NAME}"
                PREFIX ""
                SUFFIX "${_suffix}"
                LINK_FLAGS "${_link_flags}"
                POSITION_INDEPENDENT_CODE ON)

        target_include_directories(
                ${_ext_TARGET}
                PRIVATE ${PostgreSQL_SERVER_INCLUDE_DIRS}
                PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    set(_pkg_dir "${CMAKE_BINARY_DIR}/packaged")

    if(_ext_SOURCES)
        set(_target_file $<TARGET_FILE:${_ext_TARGET}>)
        set(_target_file_name $<TARGET_FILE_NAME:${_ext_TARGET}>)
    endif()

    if(DEFINED _ext_UPGRADE_SCRIPTS)
        foreach(_script ${_ext_UPGRADE_SCRIPTS})
            file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${_script} DESTINATION ${_ext_dir})
        endforeach()
    endif()

    # Generate control file at build time (which is when GENERATE evaluate the
    # contents). We do not know the target file name until then.
    set(_control_file "${_ext_dir}/${NAME}--${_ext_VERSION}.control")
    file(
            GENERATE
            OUTPUT ${_control_file}
            CONTENT
            "$<$<NOT:$<BOOL:${_ext_SOURCES}>>:#>module_pathname = '${_target_file}'
$<$<NOT:$<BOOL:${_ext_COMMENT}>>:#>comment = '${_ext_COMMENT}'
$<$<NOT:$<BOOL:${_ext_ENCODING}>>:#>encoding = '${_ext_ENCODING}'
$<$<NOT:$<BOOL:${_ext_REQUIRES}>>:#>requires = '$<JOIN:${_ext_REQUIRES},$<COMMA>>'
$<$<NOT:$<BOOL:${_ext_SCHEMA}>>:#>schema = ${_ext_SCHEMA}
$<$<NOT:$<BOOL:${_ext_RELOCATABLE}>>:#>relocatable = ${_ext_RELOCATABLE}
$<$<BOOL:${_ext_SUPERUSER}>:#>superuser = ${_ext_SUPERUSER}
            ")
    # Packaged control file
    if(NOT ${_ext_PRIVATE})
      set(_packaged_control_file "${_pkg_dir}/extension/${NAME}--${_ext_VERSION}.control")
      file(
              GENERATE
              OUTPUT ${_packaged_control_file}
              CONTENT
              "$<$<NOT:$<BOOL:${_ext_SOURCES}>>:#>module_pathname = '$libdir/${_target_file_name}'
$<$<NOT:$<BOOL:${_ext_COMMENT}>>:#>comment = '${_ext_COMMENT}'
$<$<NOT:$<BOOL:${_ext_ENCODING}>>:#>encoding = '${_ext_ENCODING}'
$<$<NOT:$<BOOL:${_ext_REQUIRES}>>:#>requires = '$<JOIN:${_ext_REQUIRES},$<COMMA>>'
$<$<NOT:$<BOOL:${_ext_SCHEMA}>>:#>schema = ${_ext_SCHEMA}
$<$<NOT:$<BOOL:${_ext_RELOCATABLE}>>:#>relocatable = ${_ext_RELOCATABLE}
$<$<BOOL:${_ext_SUPERUSER}>:#>superuser = ${_ext_SUPERUSER}
              ")
    endif()

    # Default control file
    if(NOT _ext_NO_DEFAULT_CONTROL)
    set(_default_control_file "${_ext_dir}/${NAME}.control")

    file(
        GENERATE
        OUTPUT ${_default_control_file}
        CONTENT
        "default_version = '${_ext_VERSION}'
$<$<NOT:$<BOOL:${_ext_COMMENT}>>:#>comment = '${_ext_COMMENT}'
")
    # Packaged default control file
    if(NOT ${_ext_PRIVATE})
        set(_packaged_default_control_file "${_pkg_dir}/extension/${NAME}.control")
        file(
                GENERATE
                OUTPUT ${_packaged_default_control_file}
                CONTENT
                "default_version = '${_ext_VERSION}'
$<$<NOT:$<BOOL:${_ext_COMMENT}>>:#>comment = '${_ext_COMMENT}'
")
    endif()
    endif()

    file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/script_${_ext_TARGET}
            CONTENT "#! /usr/bin/env bash
# Indicate that we've started provisioning ${NAME}
export SCRIPT_STARTED_${NAME}=1
# Dependencies
for r in $<JOIN:${_ext_REQUIRES}, > $<JOIN:${_ext_TESTS_REQUIRE}, >
do
    # If the dependency is already [being] provisioned, skip it
    DEP=\"SCRIPT_STARTED_$r\"
    if [ -n \"$\{!DEP\}\" ]; then
        continue
    fi
    if [ -f ${CMAKE_BINARY_DIR}/script_$r ]; then
        ${CMAKE_BINARY_DIR}/script_$r $1 || echo \"Skip $r\"
    else
        echo \"Skip $r\"
    fi
done
_dir=\"${CMAKE_CURRENT_SOURCE_DIR}/migrate\"
if ! [ -d \"$_dir\" ]; then
  # Skip
  exit 0
fi
if [ -d \"$_dir/${_ext_TARGET}\" ]; then
  _dir=\"$_dir/${_ext_TARGET}\"
fi
# Create file (using $$ for pid to avoid race conditions)
for f in $(ls \"$_dir/\"*.sql | sort -V)
  do
  pushd $(pwd) >/dev/null
  cd \"${CMAKE_CURRENT_SOURCE_DIR}\"
  $<TARGET_FILE:inja> \"$f\" >> \"$1/_$$_${NAME}--${_ext_VERSION}.sql\"
  echo ';' >> \"$1/_$$_${NAME}--${_ext_VERSION}.sql\"
  popd >/dev/null
done
# Move it into proper location at once
mv \"$1/_$$_${NAME}--${_ext_VERSION}.sql\" \"$1/${NAME}--${_ext_VERSION}.sql\"
# Calling another command (a kludge for testing)
if [ -z \"$2\" ]; then
  # No comamnd
  exit 0
fi
command=$2
shift 2
$command $@
"
            FILE_PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE
    )


    if (_ext_SOURCES AND NOT ${_ext_PRIVATE})
        add_custom_target(package_${_ext_TARGET}_extension
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DEPENDS omni_check_symbol_conflict_${_ext_TARGET} ${_ext_TARGET}
                COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:${_ext_TARGET}>"
                ${_pkg_dir})
        add_custom_target(install_${_ext_TARGET}_extension
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DEPENDS package_${_ext_TARGET}_extension package_${_ext_TARGET}_migrations
                COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                "${_pkg_dir}/$<TARGET_FILE_NAME:${_ext_TARGET}>"
                ${PostgreSQL_TARGET_PACKAGE_LIBRARY_DIR}
                COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                "${_pkg_dir}/extension/${NAME}.control" "${_pkg_dir}/extension/${NAME}--${_ext_VERSION}.control" "${_pkg_dir}/extension/${NAME}--${_ext_VERSION}.sql"
                ${PostgreSQL_TARGET_EXTENSION_DIR}
        )
        # Check that the extension has no conflicting symbols with the Postgres binary
        # otherwise the linker might use the symbols from Postgres
        find_package(Python COMPONENTS Interpreter REQUIRED)
        add_custom_target(omni_check_symbol_conflict_${_ext_TARGET}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/check_symbol_conflict.py ${PG_BINARY} "$<TARGET_FILE:${_ext_TARGET}>")
    endif()

    if (NOT ${_ext_PRIVATE})
        if (NOT TARGET install_${_ext_TARGET}_extension)
            add_custom_target(install_${_ext_TARGET}_extension
                    DEPENDS ${_ext_TARGET} package_${_ext_TARGET}_migrations
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMAND
                    ${CMAKE_COMMAND} -E copy_if_different
                    "${_pkg_dir}/extension/${NAME}.control" "${_pkg_dir}/extension/${NAME}--${_ext_VERSION}.control" "${_pkg_dir}/extension/${NAME}--${_ext_VERSION}.sql"
                    ${PostgreSQL_TARGET_EXTENSION_DIR}
            )
        endif ()
        add_custom_target(package_${_ext_TARGET}_migrations
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMAND ${CMAKE_BINARY_DIR}/script_${_ext_TARGET} ${_pkg_dir}/extension)
    endif ()

    if(NOT TARGET package_extensions)
        add_custom_target(package_extensions)
    endif()
    if(NOT ${_ext_PRIVATE})
        if(_ext_SOURCES)
            add_dependencies(package_extensions package_${_ext_TARGET}_extension package_${_ext_TARGET}_migrations)
        else()
            add_dependencies(package_extensions package_${_ext_TARGET}_migrations)
        endif()
    endif()

    if (NOT TARGET install_extensions)
        add_custom_target(install_extensions)
    endif ()
    if (NOT ${_ext_PRIVATE})
        add_dependencies(install_extensions install_${_ext_TARGET}_extension)
    endif ()

    if(NOT _ext_PRIVATE)
        add_custom_target(prepare_${_ext_TARGET}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMAND ${CMAKE_BINARY_DIR}/script_${_ext_TARGET} ${_ext_dir})

        if(NOT TARGET prepare)
            add_custom_target(prepare)
        endif()
        add_dependencies(prepare prepare_${_ext_TARGET})
    endif()

    if(_ext_REGRESS)
        foreach(_test ${_ext_REGRESS})
            set(_sql_file "${CMAKE_CURRENT_SOURCE_DIR}/sql/${_test}.sql")
            set(_out_file "${CMAKE_CURRENT_SOURCE_DIR}/expected/${_test}.out")

            if(NOT EXISTS "${_sql_file}")
                message(FATAL_ERROR "Test file '${_sql_file}' does not exist!")
            endif()

            if(NOT EXISTS "${_out_file}")
                file(WRITE "${_out_file}")
                message(STATUS "Created empty file ${_out_file}")
            endif()
        endforeach()

        if(PG_REGRESS)
            set(_loadextensions)
            set(_extra_config)

            foreach(requirement ${_ext_REQUIRES})
                list(APPEND _ext_TESTS_REQUIRE ${requirement})
            endforeach()

            list(REMOVE_DUPLICATES _ext_TESTS_REQUIRE)

            foreach(req ${_ext_TESTS_REQUIRE})
                string(APPEND _loadextensions "--load-extension=${req} ")

                if(req STREQUAL "omni")
                    set(_extra_config "shared_preload_libraries=\\\'$<$<TARGET_EXISTS:omni>:$<TARGET_FILE:omni>>\\\'")
                endif()
            endforeach()

            list(JOIN _ext_REGRESS " " _ext_REGRESS_ARGS)
            file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_${_ext_TARGET}
                CONTENT "#! /usr/bin/env bash
${CMAKE_BINARY_DIR}/script_${NAME} ${_ext_dir}
# Pick random port
while true
do
    export PORT=$(( ((RANDOM<<15)|RANDOM) % 49152 + 10000 ))
    status=\"$(${NETCAT} -z 127.0.0.1 $random_port < /dev/null &>/dev/null; echo $?)\"
    if [ \"${status}\" != \"0\" ]; then
        echo \"Using port $PORT\";
        break;
    fi
done
export tmpdir=$(mktemp -d)
echo local all all trust > \"$tmpdir/pg_hba.conf\"
echo host all all all trust >> \"$tmpdir/pg_hba.conf\"
echo hba_file=\\\'$tmpdir/pg_hba.conf\\\' > \"$tmpdir/postgresql.conf\"
$<IF:$<BOOL:${_ext_SHARED_PRELOAD}>,echo shared_preload_libraries='${_target_file_name}',echo> >> $tmpdir/postgresql.conf
echo ${_extra_config} >> $tmpdir/postgresql.conf
echo max_worker_processes = 64 >> $tmpdir/postgresql.conf
PGSHAREDIR=${_share_dir} \
EXTENSION_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} \
${PG_REGRESS} --temp-instance=\"$tmpdir/instance\" --inputdir=${CMAKE_CURRENT_SOURCE_DIR} \
--dbname=\"${NAME}\" \
--temp-config=\"$tmpdir/postgresql.conf\" \
--outputdir=\"${CMAKE_CURRENT_BINARY_DIR}/${NAME}\" \
${_loadextensions} \
--load-extension=${NAME} --host=0.0.0.0 --port=$PORT ${_ext_REGRESS_ARGS}
"
                FILE_PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE
            )
            add_test(
                    NAME ${_ext_TARGET}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_${_ext_TARGER}
            )
        endif()

        add_custom_target(
                ${_ext_TARGET}_update_results
                COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_BINARY_DIR}/${_ext_TARGET}/results/*.out
                ${CMAKE_CURRENT_SOURCE_DIR}/expected)
        if(NOT TARGET update_test_results)
            add_custom_target(update_test_results)
        endif()
        add_dependencies(update_test_results ${_ext_TARGET}_update_results)
    endif()

    if(_ext_TESTS OR NOT DEFINED _ext_TESTS)
        set(_tests_dir "${CMAKE_CURRENT_SOURCE_DIR}/tests")
        if(EXISTS "${_tests_dir}/${_ext_TARGET}" AND IS_DIRECTORY "${_tests_dir}/${_ext_TARGET}")
            set(_tests_dir "${_tests_dir}/${_ext_TARGET}")
        endif()
        find_pg_yregress_tests("${_tests_dir}")
    endif()

    if(INITDB AND CREATEDB AND (PSQL OR PGCLI) AND PG_CTL)
        if(PGCLI)
            set(_cli ${PGCLI})
        else()
            set(_cli ${PSQL})
        endif()

        file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/psql_${_ext_TARGET}
            CONTENT "#! /usr/bin/env bash
${CMAKE_BINARY_DIR}/script_${_ext_TARGET} ${_ext_dir}
if [ -z \"$PGPORT\" ]; then
    # Try 5432 first
    export PGPORT=5432
    while true
    do
        status=\"$(${NETCAT} -z 127.0.0.1 $PGPORT < /dev/null &>/dev/null; echo $?)\"
        if [[ \"$status\" != \"0\" ]]; then
            echo \"Using port $PGPORT\";
            break;
        fi
        # Pick random port
        export PGPORT=$(( ((RANDOM<<15)|RANDOM) % 49152 + 10000 ))
    done
fi
export EXTENSION_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
if [ -z \"$PSQLDB\" ]; then
   mkdir -p \"${CMAKE_CURRENT_BINARY_DIR}/data\"
   PSQLDB=\"$(mktemp -d ${CMAKE_CURRENT_BINARY_DIR}/data/${NAME}.XXXXXX)\"
   rm -rf \"$PSQLDB\"
fi
if [ \"$(shopt -s nullglob; shopt -s dotglob; files=($PSQLDB/*); echo $\{#files[@]\})\" -gt 0 ]; then
    echo PSQLDB is not empty, skipping initialization
else
    ${INITDB} -D \"$PSQLDB\" --no-clean --no-sync --locale=C --encoding=UTF8
fi
rm -f \"$PSQLDB/standby.signal\"
export SOCKDIR=$(mktemp -d)
if [ -z \"$POSTGRESQLCONF\" ]; then
  if [ -f \"${CMAKE_CURRENT_SOURCE_DIR}/postgresql.conf\" ]; then
    export POSTGRESQLCONF=\"${CMAKE_CURRENT_SOURCE_DIR}/postgresql.conf\"
  fi
fi
echo \"Using postgresql.conf: $POSTGRESQLCONF\"
if [ -z \"$PGHBACONF\" ]; then
 if [ -f \"${CMAKE_CURRENT_SOURCE_DIR}/pg_hba.conf\" ]; then
    export PGHBACONF=\"${CMAKE_CURRENT_SOURCE_DIR}/pg_hba.conf\"
  fi
fi
if [ -n \"$POSTGRESQLCONF\" ]; then
 (cat \"$POSTGRESQLCONF\"  || exit 1) > \"$PSQLDB/postgresql.conf\"
fi
if [ -n \"$PGHBACONF\" ]; then
  echo \"Using pg_hba.conf: $PGBACONF\"
  cp \"$PGHBACONF\" \"$PSQLDB/pg_hba.conf\" || exit 1
else
  echo host all all all trust >>  \"$PSQLDB/pg_hba.conf\"
fi
PGSHAREDIR=${_share_dir} \
${PG_CTL} start -D \"$PSQLDB\" \
-o \"-c max_worker_processes=64 -c listen_addresses=* -c port=$PGPORT $<IF:$<BOOL:${_ext_SHARED_PRELOAD}>,-c shared_preload_libraries='${_target_file}$<COMMA>$<$<TARGET_EXISTS:omni>:$<TARGET_FILE:omni>>',-c shared_preload_libraries='$<$<TARGET_EXISTS:omni>:$<TARGET_FILE:omni>>'>\" \
-o -F -o -k -o \"$SOCKDIR\"
${CREATEDB} -h \"$SOCKDIR\" ${NAME}
if [ -z \"$PSQLRC\" ]; then
  if [ -f \"${CMAKE_CURRENT_SOURCE_DIR}/.psqlrc\" ]; then
    export PSQLRC=\"${CMAKE_CURRENT_SOURCE_DIR}/.psqlrc\"
  fi
else
  echo \"Using supplied .psqlrc: $PSQLRC\"
  export PSQLRC
fi
if [ -n \"$STANDBY\" ]; then
  touch \"$PSQLDB/standby.signal\"
  echo \"hot_standby = on\" >> \"$PSQLDB/postgresql.conf\"
  echo \"restore_command = 'false'\" >> \"$PSQLDB/postgresql.conf\"
  ${_cli} --set=CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -h \"$SOCKDIR\" ${NAME} -c select
  PGSHAREDIR=${_share_dir} ${PG_CTL} restart -D  \"$PSQLDB\"
  unset PSQLRC
fi
${_cli} --set=CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -h \"$SOCKDIR\" ${NAME}
${PG_CTL} stop -D  \"$PSQLDB\" -m smart
"
                FILE_PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE
                )
        add_custom_target(psql_${_ext_TARGET}
                WORKING_DIRECTORY "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.."
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/psql_${_ext_TARGET})
        add_dependencies(psql_${_ext_TARGET} ${_ext_TARGET} prepare omni)
    endif()

    if(PG_REGRESS)
        # We add a custom target to get output when there is a failure.
        add_custom_target(
                test_verbose_${_ext_TARGET} COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process
                --verbose --output-on-failure)
    endif()

    if(NOT _ext_PRIVATE)
        get_property(omni_artifacts GLOBAL PROPERTY omni_artifacts)
        if(NOT omni_artifacts)
            # Make an empty file
            set_property(GLOBAL PROPERTY omni_artifacts "${CMAKE_BINARY_DIR}/artifacts.txt")
            get_property(omni_artifacts GLOBAL PROPERTY omni_artifacts)
            file(WRITE "${omni_artifacts}" "")
        endif()
        file(APPEND "${omni_artifacts}" "${NAME}=${_ext_VERSION}")

        list(LENGTH _ext_REQUIRES len)
        if(${len} GREATER 0)
            file(APPEND "${omni_artifacts}" "#")
            set(_ctr 0)
            foreach(requirement ${_ext_REQUIRES})
                math(EXPR _ctr "${_ctr} + 1")
                if(TARGET ${requirement})
                    get_version(${requirement} _ver)
                else()
                    set(_ver "*")
                endif()
                file(APPEND "${omni_artifacts}" "${requirement}=${_ver}")
                if(_ctr LESS len)
                    file(APPEND "${omni_artifacts}" ",")
                endif()
            endforeach()
        endif()

        file(APPEND "${omni_artifacts}" "\n")
    endif()

    if(NOT _ext_PRIVATE)
        get_property(omni_path_map GLOBAL PROPERTY omni_path_map)
        if(NOT omni_path_map)
            # Make an empty file
            set_property(GLOBAL PROPERTY omni_path_map "${CMAKE_BINARY_DIR}/paths.txt")
            get_property(omni_path_map GLOBAL PROPERTY omni_path_map)
            file(WRITE "${omni_path_map}" "")
        endif()
        set(relpath ${CMAKE_CURRENT_SOURCE_DIR})
        cmake_path(RELATIVE_PATH relpath BASE_DIRECTORY ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..)
        file(APPEND "${omni_path_map}" "${NAME} ${relpath}\n")
    endif()

endfunction()
