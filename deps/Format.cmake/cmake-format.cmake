# Regex-filter a git repository's files.
function(get_cmake_files)
  cmake_parse_arguments("" "" "GIT_REPOSITORY_DIR;OUTPUT_LIST;REGEX" "" ${ARGN})
  execute_process(
    COMMAND ${GIT_PROGRAM} ls-files --cached --exclude-standard ${_GIT_REPOSITORY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE all_files
  )
  cmake_policy(SET CMP0007 NEW)
  string(REPLACE "\n" ";" filtered_files "${all_files}")
  list(FILTER filtered_files INCLUDE REGEX ${_REGEX})
  foreach(it ${filtered_files})
    if(NOT EXISTS ${it} OR IS_DIRECTORY ${it})
      list(REMOVE_ITEM filtered_files ${it})
    endif()
  endforeach()
  if(CMAKE_FORMAT_EXCLUDE)
    list(FILTER filtered_files EXCLUDE REGEX ${CMAKE_FORMAT_EXCLUDE})
  endif()
  set(${_OUTPUT_LIST}
      ${filtered_files}
      PARENT_SCOPE
  )
endfunction()

execute_process(
  COMMAND git rev-parse --show-toplevel
  OUTPUT_VARIABLE GIT_TOPLEVEL
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# remove trailing whitespace from output
string(STRIP ${GIT_TOPLEVEL} GIT_TOPLEVEL)

get_cmake_files(
  GIT_REPOSITORY_DIR ${GIT_TOPLEVEL} OUTPUT_LIST CMAKE_FILES REGEX
  "\\.cmake$|(^|/)CMakeLists\\.txt$"
)

separate_arguments(CMAKE_FORMAT_EXTRA_ARGS)

if(CMAKE_FORMAT_TARGET STREQUAL fix-cmake-format)
  execute_process(COMMAND ${CMAKE_FORMAT_PROGRAM} ${CMAKE_FORMAT_EXTRA_ARGS} -i ${CMAKE_FILES})
  return()
endif()

if(CMAKE_FORMAT_TARGET STREQUAL check-cmake-format)
  set(OUTPUT_QUIET_OPTION OUTPUT_QUIET)
endif()

set(formatted_cmake_file ${BINARY_DIR}/formatted.cmake)
foreach(cmake_file IN LISTS CMAKE_FILES)
  set(source_cmake_file ${CMAKE_SOURCE_DIR}/${cmake_file})
  execute_process(
    COMMAND ${CMAKE_FORMAT_PROGRAM} ${CMAKE_FORMAT_EXTRA_ARGS} -o ${formatted_cmake_file}
            ${source_cmake_file}
  )

  execute_process(
    COMMAND ${GIT_PROGRAM} diff -G. --color --no-index -- ${source_cmake_file}
            ${formatted_cmake_file} RESULT_VARIABLE result ${OUTPUT_QUIET_OPTION}
  )
  if(OUTPUT_QUIET_OPTION AND result)
    message(FATAL_ERROR "${cmake_file} needs to be reformatted")
  endif()
endforeach()
