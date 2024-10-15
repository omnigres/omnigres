option(FORMAT_SKIP_CMAKE "Skip CMake formatting.")
option(FORMAT_SKIP_CLANG "Skip Clang formatting.")

find_program(CLANG_FORMAT_PROGRAM clang-format)
find_program(GIT_PROGRAM git)
find_program(CMAKE_FORMAT_PROGRAM cmake-format)
find_package(Python)

if(CLANG_FORMAT_PROGRAM AND Python_FOUND)
  set(CLANG_FORMAT_COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/git-clang-format.py
                           --binary=${CLANG_FORMAT_PROGRAM} "--ignored-paths=${CLANG_FORMAT_IGNORED_PATHS}"
  )
  set(GIT_EMPTY_TREE_HASH 4b825dc642cb6eb9a060e54bf8d69288fbee4904)

  add_custom_target(
    clang-format
    COMMAND ${CLANG_FORMAT_COMMAND} --diff ${GIT_EMPTY_TREE_HASH}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )

  add_custom_target(
    check-clang-format
    COMMAND ${CLANG_FORMAT_COMMAND} --ci ${GIT_EMPTY_TREE_HASH}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )

  add_custom_target(
    fix-clang-format
    COMMAND ${CLANG_FORMAT_COMMAND} ${GIT_EMPTY_TREE_HASH} -f
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
else()
  message(STATUS "Format.cmake: clang-format and/or python not found, adding dummy targets")

  set(CLANG_FORMAT_NOT_FOUND_COMMAND_ARGS
      # show error message
      COMMAND ${CMAKE_COMMAND} -E echo
      "Format.cmake: cannot run because clang-format and/or python not found"
      # fail build
      COMMAND ${CMAKE_COMMAND} -E false
  )

  add_custom_target(clang-format ${CLANG_FORMAT_NOT_FOUND_COMMAND_ARGS})
  add_custom_target(check-clang-format ${CLANG_FORMAT_NOT_FOUND_COMMAND_ARGS})
  add_custom_target(fix-clang-format ${CLANG_FORMAT_NOT_FOUND_COMMAND_ARGS})
endif()

if(GIT_PROGRAM AND CMAKE_FORMAT_PROGRAM)
  function(add_cmake_format_target name)
    add_custom_target(
      ${name}
      COMMAND
        ${CMAKE_COMMAND} -DGIT_PROGRAM=${GIT_PROGRAM} -DCMAKE_FORMAT_PROGRAM=${CMAKE_FORMAT_PROGRAM}
        -DCMAKE_FORMAT_EXTRA_ARGS=${CMAKE_FORMAT_EXTRA_ARGS} -DCMAKE_FORMAT_TARGET=${name}
        -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR} -DCMAKE_FORMAT_EXCLUDE=${CMAKE_FORMAT_EXCLUDE} -P
        ${CMAKE_CURRENT_LIST_DIR}/cmake-format.cmake
      VERBATIM
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
  endfunction()

  add_cmake_format_target(cmake-format)
  add_cmake_format_target(check-cmake-format)
  add_cmake_format_target(fix-cmake-format)
else()
  message(STATUS "Format.cmake: cmake-format and/or git not found, adding dummy targets")

  set(CMAKE_FORMAT_NOT_FOUND_COMMAND_ARGS
      # show error message
      COMMAND ${CMAKE_COMMAND} -E echo
      "Format.cmake: cannot run because cmake-format and/or git not found"
      # fail build
      COMMAND ${CMAKE_COMMAND} -E false
  )

  add_custom_target(cmake-format ${CMAKE_FORMAT_NOT_FOUND_COMMAND_ARGS})
  add_custom_target(check-cmake-format ${CMAKE_FORMAT_NOT_FOUND_COMMAND_ARGS})
  add_custom_target(fix-cmake-format ${CMAKE_FORMAT_NOT_FOUND_COMMAND_ARGS})
endif()

if(NOT FORMAT_SKIP_CLANG)
  list(APPEND FORMAT_TARGETS clang-format)
  list(APPEND CHECK_FORMAT_TARGETS check-clang-format)
  list(APPEND FIX_FORMAT_TARGETS fix-clang-format)
endif()
if(NOT FORMAT_SKIP_CMAKE)
  list(APPEND FORMAT_TARGETS cmake-format)
  list(APPEND CHECK_FORMAT_TARGETS check-cmake-format)
  list(APPEND FIX_FORMAT_TARGETS fix-cmake-format)
endif()

add_custom_target(format)
add_custom_target(check-format)
add_custom_target(fix-format)

if(NOT FORMAT_SKIP_CLANG AND NOT FORMAT_SKIP_CMAKE)
  add_dependencies(format ${FORMAT_TARGETS})
  add_dependencies(check-format ${CHECK_FORMAT_TARGETS})
  add_dependencies(fix-format ${FIX_FORMAT_TARGETS})
endif()
