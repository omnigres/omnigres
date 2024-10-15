# CUNIT_INCLUDE_DIRS - where to find <CUnit/CUnit.h>, etc.
# CUNIT_LIBRARIES - List of libraries when using libcunit.
# CUNIT_FOUND - True if libcunit found.
if(CUNIT_INCLUDE_DIRS)
	# Already in cache, be silent
	set(CUNIT_FIND_QUIETLY YES)
endif()

find_path(CUNIT_INCLUDE_DIRS CUnit/CUnit.h)
find_library(CUNIT_LIBRARY NAMES cunit libcunit)

# handle the QUIETLY and REQUIRED arguments and set CUNIT_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CUNIT DEFAULT_MSG CUNIT_LIBRARY CUNIT_INCLUDE_DIRS)

if(CUNIT_FOUND)
	set(CUNIT_LIBRARIES ${CUNIT_LIBRARY})
endif()
