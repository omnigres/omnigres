cmake_minimum_required(VERSION 3.2)

find_package(CUnit REQUIRED)

# Choose a wslay target to get build options from.
# That way we can avoid duplicating them here.
if(WSLAY_STATIC)
	set(WSLAY_TARGET "wslay")
else()
	set(WSLAY_TARGET "wslay_shared")
endif()

get_target_property(WSLAY_INCLUDE_DIRS
	${WSLAY_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(WSLAY_COMPILE_DEFINITIONS
	${WSLAY_TARGET} COMPILE_DEFINITIONS)
set(WSLAY_DIR ${PROJECT_SOURCE_DIR}/lib)
set(WSLAY_HEADERS
	${WSLAY_DIR}/wslay_event.h
	${WSLAY_DIR}/wslay_frame.h
	${WSLAY_DIR}/wslay_net.h
	${WSLAY_DIR}/wslay_queue.h)
set(WSLAY_SOURCES
	${WSLAY_DIR}/wslay_event.c
	${WSLAY_DIR}/wslay_frame.c
	${WSLAY_DIR}/wslay_net.c
	${WSLAY_DIR}/wslay_queue.c)

set(HEADERS
	wslay_event_test.h
	wslay_frame_test.h
	wslay_queue_test.h
	wslay_session_test.h)
set(SOURCES
	wslay_event_test.c
	wslay_frame_test.c
	wslay_queue_test.c
	wslay_session_test.c
	main.c)
add_executable(wslay_tests ${SOURCES} ${HEADERS}
	${WSLAY_SOURCES} ${WSLAY_HEADERS})
target_include_directories(wslay_tests PRIVATE ${WSLAY_DIR} ${WSLAY_INCLUDE_DIRS} ${CUNIT_INCLUDE_DIRS})
target_compile_definitions(wslay_tests PRIVATE ${WSLAY_COMPILE_DEFINITIONS})
target_link_libraries(wslay_tests ${CUNIT_LIBRARIES})
add_dependencies(wslay_tests ${WSLAY_TARGET})
add_test(NAME wslay_tests COMMAND wslay_tests)
