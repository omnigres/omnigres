cmake_minimum_required(VERSION 3.25.1)
project(libpgaug)

if(NOT DEFINED ENV{LIBPGAUG_ALREADY_INCLUDED})
        set(ENV{LIBPGAUG_ALREADY_INCLUDED} TRUE)

        include(CTest)

        list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

        enable_testing()

        try_compile(HAVE_ATTRIBUTE_CLEANUP SOURCE_FROM_CONTENT attribute_cleanup.c [=[
void cleanup(int *i) {}
int main() {
__attribute__((cleanup(cleanup))) int i = 0;
}
]=])

        if(NOT ${HAVE_ATTRIBUTE_CLEANUP})
                message(SEND_ERROR "libpgaug requires __attribute__((cleanup)) support")
        endif()

        find_package(PostgreSQL REQUIRED)

        add_library(libpgaug STATIC libpgaug.c)
        set_property(TARGET libpgaug PROPERTY POSITION_INDEPENDENT_CODE ON)
        target_include_directories(libpgaug
                PRIVATE ${PostgreSQL_SERVER_INCLUDE_DIRS}
                PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

        add_subdirectory(test)
endif()