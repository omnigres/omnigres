include(CPM)
include(FindPkgConfig)

pkg_check_modules (STC STC)

if (NOT STC_FOUND)
  CPMAddPackage(NAME stc GIT_REPOSITORY https://github.com/stclib/STC GIT_TAG v4.2 VERSION 4.2 OPTIONS "BUILD_TESTING OFF")
endif()
