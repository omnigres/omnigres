if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW) # Allow project(xxx VERSION a.b.c)
endif()

# version number below should be the currently under development version,
# so when doing a tag/release it is captured with the proper numbering.
project(fyaml LANGUAGES C VERSION 0.7.2)

cmake_minimum_required(VERSION 3.0)

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

set(LIBHDRS
  src/valgrind/fy-valgrind.h
  src/xxhash/xxhash.h
  src/lib/fy-accel.h
  src/lib/fy-atom.h
  src/lib/fy-composer.h
  src/lib/fy-ctype.h
  src/lib/fy-diag.h
  src/lib/fy-doc.h
  src/lib/fy-docbuilder.c
  src/lib/fy-docstate.h
  src/lib/fy-dump.h
  src/lib/fy-emit-accum.h
  src/lib/fy-emit.h
  src/lib/fy-event.h
  src/lib/fy-input.h
  src/lib/fy-list.h
  src/lib/fy-parse.h
  src/lib/fy-path.h
  src/lib/fy-token.h
  src/lib/fy-typelist.h
  src/lib/fy-types.h
  src/lib/fy-utf8.h
  src/lib/fy-utils.h
  src/lib/fy-walk.h
)
set(LIBSRCS
  src/lib/fy-accel.c
  src/lib/fy-atom.c
  src/lib/fy-composer.c
  src/lib/fy-ctype.c
  src/lib/fy-diag.c
  src/lib/fy-doc.c
  src/lib/fy-docstate.c
  src/lib/fy-dump.c
  src/lib/fy-emit.c
  src/lib/fy-event.c
  src/lib/fy-input.c
  src/lib/fy-parse.c
  src/lib/fy-path.c
  src/lib/fy-token.c
  src/lib/fy-types.c
  src/lib/fy-utf8.c
  src/lib/fy-utils.c
  src/lib/fy-walk.c
  src/xxhash/xxhash.c
)
set(LIBHDRSPUB
    include/libfyaml.h
)

add_library(${PROJECT_NAME}
    ${LIBHDRS}
    ${LIBSRCS}
    ${LIBHDRSPUB}
)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${${PROJECT_NAME}_SOURCE_DIR}/src/lib
        ${${PROJECT_NAME}_SOURCE_DIR}/src/lib/internal
        ${${PROJECT_NAME}_SOURCE_DIR}/src/xxhash
)

#target_link_libraries(${PROJECT_NAME} PUBLIC xx)
#target_compile_definitions(${PROJECT_NAME} PUBLIC xx)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VERSION="0.5.7.34"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  $<$<NOT:$<CONFIG:DEBUG>>:NDEBUG>
)

# Shared options between GCC and CLANG:
if (NOT MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -fPIC
        -D_GNU_SOURCE
    )

	if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
		target_compile_options(${PROJECT_NAME} PRIVATE
            -O2
        )
	endif()
endif()

# ==== Install & export target ========

install(TARGETS	${PROJECT_NAME}
		EXPORT 	${PROJECT_NAME}-targets
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT ${PROJECT_NAME}-targets
    FILE ${PROJECT_NAME}-targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
export(
	TARGETS ${PROJECT_NAME}
	FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}-targets.cmake"
	NAMESPACE ${PROJECT_NAME}::
)


include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
