cmake_minimum_required(VERSION 3.2)

include(CheckIncludeFiles)
check_include_files("sys/epoll.h" HAVE_EPOLL)
if(NOT HAVE_EPOLL)
	message(FATAL_ERROR "epoll is required")
endif()
find_package(Nettle REQUIRED)

if(WSLAY_STATIC)
	set(WSLAY_TARGET "wslay")
else()
	set(WSLAY_TARGET "wslay_shared")
endif()

add_executable(echoserv echoserv.cc)
target_include_directories(echoserv PRIVATE ${NETTLE_INCLUDE_DIRS})
target_link_libraries(echoserv ${WSLAY_TARGET} ${NETTLE_LIBRARIES})

add_executable(fork-echoserv fork-echoserv.c)
target_include_directories(fork-echoserv PRIVATE ${NETTLE_INCLUDE_DIRS})
target_link_libraries(fork-echoserv ${WSLAY_TARGET} ${NETTLE_LIBRARIES})

add_executable(testclient testclient.cc)
target_include_directories(testclient PRIVATE ${NETTLE_INCLUDE_DIRS})
target_link_libraries(testclient ${WSLAY_TARGET} ${NETTLE_LIBRARIES})
