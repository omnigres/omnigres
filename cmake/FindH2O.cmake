include(CPM)
include(CheckCSourceCompiles)

set(_h2o_options)
list(APPEND _h2o_options "WITH_MRUBY OFF")
list(APPEND _h2o_options "DISABLE_LIBUV ON")

cmake_policy(SET CMP0042 NEW)
# Using a fork because of https://github.com/h2o/h2o/issues/3243
CPMAddPackage(NAME h2o GIT_REPOSITORY https://github.com/omnigres/h2o GIT_TAG 08057ce19 VERSION 2.3.0-08057ce19 OPTIONS "${_h2o_options}")
set_property(TARGET libh2o PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET libh2o-evloop PROPERTY POSITION_INDEPENDENT_CODE ON)
