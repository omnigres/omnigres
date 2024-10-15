# NETTLE_INCLUDE_DIRS - where to find <nettle/sha.h>, etc.
# NETTLE_LIBRARIES - List of libraries when using libnettle.
# NETTLE_FOUND - True if libnettle found.
if(NETTLE_INCLUDE_DIRS)
	# Already in cache, be silent
	set(NETTLE_FIND_QUIETLY YES)
endif()

find_path(NETTLE_INCLUDE_DIRS nettle/md5.h nettle/ripemd160.h nettle/sha.h)
find_library(NETTLE_LIBRARY NAMES nettle libnettle)

# handle the QUIETLY and REQUIRED arguments and set NETTLE_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NETTLE DEFAULT_MSG NETTLE_LIBRARY NETTLE_INCLUDE_DIRS)

if(NETTLE_FOUND)
	set(NETTLE_LIBRARIES ${NETTLE_LIBRARY})
endif()
