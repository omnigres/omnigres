cmake_minimum_required(VERSION 2.8)

# This CMakeLists.txt file intended for:
# - include by master ../CMakeLists.txt
# - embedding wslay library into other projects

set(PACKAGE_VERSION "1.0.1-DEV")
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/includes)
set(GEN_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/includes)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -pedantic-errors -Wno-long-long")

include(CheckIncludeFile)
include(TestBigEndian)
check_include_file("arpa/inet.h" HAVE_ARPA_INET_H)
check_include_file("netinet/in.h" HAVE_NETINET_IN_H)
check_include_file("winsock2.h" HAVE_WINSOCK2_H)
test_big_endian(WORDS_BIGENDIAN)

configure_file(${INCLUDE_DIR}/wslay/wslayver.h.in
	${GEN_INCLUDE_DIR}/wslay/wslayver.h @ONLY)
configure_file(config.h.in config.h @ONLY)

set(HEADERS
	wslay_event.h
	wslay_frame.h
	wslay_net.h
	wslay_queue.h
	${INCLUDE_DIR}/wslay/wslay.h
	${GEN_INCLUDE_DIR}/wslay/wslayver.h)
set(SOURCES
	wslay_event.c
	wslay_frame.c
	wslay_net.c
	wslay_queue.c)

set(WSLAY_TARGETS)
if(WSLAY_STATIC)
	add_library(wslay STATIC ${SOURCES} ${HEADERS})
	list(APPEND WSLAY_TARGETS wslay)
endif()
if(WSLAY_SHARED)
	add_library(wslay_shared SHARED ${SOURCES} ${HEADERS})
	list(APPEND WSLAY_TARGETS wslay_shared)
endif()

foreach(target ${WSLAY_TARGETS})
	set_property(TARGET ${target} PROPERTY C_STANDARD 99)
	target_include_directories(${target} PUBLIC
		$<BUILD_INTERFACE:${INCLUDE_DIR}>
		$<BUILD_INTERFACE:${GEN_INCLUDE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
	target_compile_definitions(${target} PRIVATE "HAVE_CONFIG_H")
endforeach()

# install
if(WSLAY_CONFIGURE_INSTALL)
	include(GNUInstallDirs)
	install(TARGETS ${WSLAY_TARGETS} EXPORT wslay
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
	install(DIRECTORY ${INCLUDE_DIR}/ ${GEN_INCLUDE_DIR}/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		FILES_MATCHING PATTERN "*.h")
endif()
