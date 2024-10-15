cmake_minimum_required(VERSION 2.8)

# This CMakeLists.txt file intended for:
# - development of wslay library
# - building wslay for install purposes
# - building wslay with ExternalProject_Add()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
endif()

project(wslay)

option(WSLAY_CONFIGURE_INSTALL "Generate install target" ON)
option(WSLAY_STATIC "Build static version of the library" ON)
option(WSLAY_SHARED "Build shared version of the library" OFF)
option(WSLAY_EXAMPLES "Build examples" OFF)
option(WSLAY_TESTS "Build tests" OFF)

add_subdirectory(lib)
if(WSLAY_EXAMPLES)
	add_subdirectory(examples)
endif()
if(WSLAY_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

if (WSLAY_CONFIGURE_INSTALL)
	include(GNUInstallDirs)
	set(INSTALL_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/wslay)
	install(EXPORT wslay
		DESTINATION ${INSTALL_CMAKE_DIR})
	configure_file(wslay-config.cmake.in wslay-config.cmake @ONLY)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wslay-config.cmake
		DESTINATION ${INSTALL_CMAKE_DIR})
endif()
