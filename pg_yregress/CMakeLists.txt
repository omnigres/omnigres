cmake_minimum_required(VERSION 3.25.1)
project(pg_yregress)

find_package(PostgreSQL REQUIRED)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

include(CPM)
CPMAddPackage(GITHUB_REPOSITORY "pantoniou/libfyaml" GIT_TAG "v0.8" OPTIONS "BUILD_SHARED_LIBS OFF")

include(CTest)

add_executable(pg_yregress pg_yregress.c instance.c test.c sighandler.c str.c)
target_link_libraries(pg_yregress PRIVATE fyaml pq)

# Enable GNU extensions for `asprintf`, `nftw`, etc.
if(NOT APPLE)
    target_compile_definitions(pg_yregress PUBLIC _GNU_SOURCE)
endif()

target_include_directories(pg_yregress PRIVATE ${PostgreSQL_INCLUDE_DIRS})
target_link_directories(pg_yregress PRIVATE ${PostgreSQL_LIBRARY_DIRS})

enable_testing()
add_test(NAME pg_yregress COMMAND "$<TARGET_FILE:pg_yregress>" "${CMAKE_CURRENT_LIST_DIR}/test.yml")
set_property(TEST pg_yregress PROPERTY ENVIRONMENT "PGCONFIG=${PG_CONFIG}")