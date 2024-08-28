cmake_minimum_required(VERSION 3.25.1)
project(omnigres_dependencies)

include(../CPM.cmake)
include(versions.cmake)

CPMAddPackage(NAME h2o GIT_REPOSITORY https://github.com/h2o/h2o GIT_TAG ${GIT_TAG_h2o} VERSION ${VERSION_h2o} DOWNLOAD_ONLY YES)
CPMAddPackage(NAME pugixml GIT_REPOSITORY https://github.com/zeux/pugixml VERSION ${VERSION_pugixml} DOWNLOAD_ONLY YES)
CPMAddPackage(NAME metalang99 GIT_REPOSITORY https://github.com/Hirrolot/metalang99 VERSION ${VERSION_metalang} DOWNLOAD_ONLY YES)
CPMAddPackage(NAME stc GIT_REPOSITORY https://github.com/stclib/STC GIT_TAG ${GIT_TAG_stc} VERSION ${VERSION_stc} DOWNLOAD_ONLY YES)
CPMAddPackage(NAME uriparser GIT_REPOSITORY https://github.com/uriparser/uriparser GIT_TAG ${GIT_TAG_uriparser} VERSION ${VERSION_uriparser} DOWNLOAD_ONLY YES)
CPMAddPackage(NAME wslay GIT_REPOSITORY https://github.com/tatsuhiro-t/wslay GIT_TAG ${GIT_TAG_wslay} VERSION ${VERSION_wslay} DOWNLOAD_ONLY YES)
CPMAddPackage(NAME libfyaml GITHUB_REPOSITORY "yrashk/libfyaml" GIT_TAG ${GIT_TAG_libfyaml} VERSION ${VERSION_libfyaml} DOWNLOAD_ONLY YES)

set(ENV{VIRTUAL_ENV} "${CMAKE_BINARY_DIR}/venv")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" -m venv $ENV{VIRTUAL_ENV})

set(Python3_FIND_VIRTUALENV FIRST)
unset(Python3_EXECUTABLE)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

execute_process(COMMAND "${Python3_EXECUTABLE}" -m pip download build -d "${CMAKE_BINARY_DIR}/pip-downloads")
execute_process(COMMAND "${Python3_EXECUTABLE}" -m pip download "setuptools>=40.8.0" -d "${CMAKE_BINARY_DIR}/pip-downloads")
execute_process(COMMAND "${Python3_EXECUTABLE}" -m pip download
        -r "${CMAKE_CURRENT_LIST_DIR}/../../docs/requirements.txt" -r "${CMAKE_CURRENT_LIST_DIR}/../../tools/requirements.txt"
        -d "${CMAKE_BINARY_DIR}/pip-downloads")

file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/pip.conf CONTENT "
[global]
no-index = true
find-links = ${CMAKE_BINARY_DIR}/pip-downloads
")