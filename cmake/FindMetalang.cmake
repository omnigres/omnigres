include(CPM)
include(FindPkgConfig)

pkg_check_modules(metalang99 metalang99)

if(NOT metalang99_FOUND)
  CPMAddPackage("gh:Hirrolot/metalang99@1.13.2")
endif()
