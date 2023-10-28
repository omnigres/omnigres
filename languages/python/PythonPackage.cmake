function(_normalize_package_name input_string output_variable)
    string(REGEX REPLACE "[._-]+" "-" modified_string "${input_string}")
    set(${output_variable} "${modified_string}" PARENT_SCOPE)
endfunction()

function(add_python_package NAME)
    set(_optional)
    set(_multi SOURCES)
    set(_single VERSION PACKAGE_NAME)
    cmake_parse_arguments(_py "${_optional}" "${_single}" "${_multi}" ${ARGN})

    set(ENV{VIRTUAL_ENV} "${CMAKE_CURRENT_BINARY_DIR}/venv")

    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    execute_process(COMMAND "${Python3_EXECUTABLE}" -m venv $ENV{VIRTUAL_ENV})

    set(Python3_FIND_VIRTUALENV FIRST)
    unset(Python3_EXECUTABLE)
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

    execute_process(COMMAND "${Python3_EXECUTABLE}" -m pip install --upgrade build)

    # List of files that, when changed, will trigger the custom command
    set(PYTHON_PACKAGE_FILES
            ${_py_SOURCES}
            )

    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dist/${_py_PACKAGE_NAME}-${_py_VERSION}.tar.gz ${CMAKE_CURRENT_BINARY_DIR}/dist/${_py_PACKAGE_NAME}-${_py_VERSION}-py3-none-any.whl
            COMMAND ${Python3_EXECUTABLE} -m build --outdir ${CMAKE_CURRENT_BINARY_DIR}/dist
            DEPENDS ${PYTHON_PACKAGE_FILES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_custom_target(
            ${NAME} ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dist/${_py_PACKAGE_NAME}-${_py_VERSION}.tar.gz ${CMAKE_CURRENT_BINARY_DIR}/dist/${_py_PACKAGE_NAME}-${_py_VERSION}-py3-none-any.whl
    )

    _normalize_package_name(${_py_PACKAGE_NAME} _py_NAME_normalized)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/python-index/${_py_NAME_normalized}/${_py_PACKAGE_NAME}-${_py_VERSION}.tar.gz ${CMAKE_BINARY_DIR}/python-index/${_py_NAME_normalized}/${_py_PACKAGE_NAME}-${_py_VERSION}-py3-none-any.whl
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/dist/${_py_PACKAGE_NAME}-${_py_VERSION}.tar.gz ${CMAKE_BINARY_DIR}/python-index/${_py_NAME_normalized}/${_py_PACKAGE_NAME}-${_py_VERSION}.tar.gz
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/dist/${_py_PACKAGE_NAME}-${_py_VERSION}-py3-none-any.whl ${CMAKE_BINARY_DIR}/python-index/${_py_NAME_normalized}/${_py_PACKAGE_NAME}-${_py_VERSION}-py3-none-any.whl
            DEPENDS ${NAME} ${CMAKE_CURRENT_BINARY_DIR}/dist/${_py_PACKAGE_NAME}-${_py_VERSION}.tar.gz ${CMAKE_CURRENT_BINARY_DIR}/dist/${_py_PACKAGE_NAME}-${_py_VERSION}-py3-none-any.whl)

    add_custom_target(
            ${NAME}_indexed ALL
            DEPENDS ${CMAKE_BINARY_DIR}/python-index/${_py_NAME_normalized}/${_py_PACKAGE_NAME}-${_py_VERSION}.tar.gz ${CMAKE_BINARY_DIR}/python-index/${_py_NAME_normalized}/${_py_PACKAGE_NAME}-${_py_VERSION}-py3-none-any.whl
    )

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/python-wheels/${_py_PACKAGE_NAME}-${_py_VERSION}-py3-none-any.whl
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/dist/${_py_PACKAGE_NAME}-${_py_VERSION}-py3-none-any.whl ${CMAKE_BINARY_DIR}/python-wheels/${_py_PACKAGE_NAME}-${_py_VERSION}-py3-none-any.whl
            DEPENDS ${NAME} ${CMAKE_CURRENT_BINARY_DIR}/dist/${_py_PACKAGE_NAME}-${_py_VERSION}-py3-none-any.whl)

    add_custom_target(
            ${NAME}_wheeled ALL
            DEPENDS ${CMAKE_BINARY_DIR}/python-wheels/${_py_PACKAGE_NAME}-${_py_VERSION}-py3-none-any.whl
    )



    if(NOT TARGET pip_index)
        add_custom_target(pip_index ALL)
    endif()
    add_dependencies(pip_index ${NAME}_indexed)

    if(NOT TARGET wheels_dir)
        add_custom_target(wheels_dir ALL)
    endif()
    add_dependencies(wheels_dir ${NAME}_wheeled)
endfunction()