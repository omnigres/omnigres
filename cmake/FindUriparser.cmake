include(CPM)
include(CheckCSourceCompiles)
include(FindPkgConfig)

pkg_check_modules (uriparser liburiparser)

if(NOT uriparser_FOUND)
  CPMAddPackage(NAME uriparser GIT_REPOSITORY https://github.com/uriparser/uriparser GIT_TAG uriparser-0.9.7 VERSION 0.9.7
                OPTIONS "BUILD_SHARED_LIBS OFF" "URIPARSER_BUILD_DOCS OFF" "URIPARSER_BUILD_TESTS OFF" "URIPARSER_BUILD_TOOLS OFF"
                        "URIPARSER_ENABLE_INSTALL OFF" "URIPARSER_WARNINGS_AS_ERRORS ON")
  set_property(TARGET uriparser PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()
