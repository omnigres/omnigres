cmake_minimum_required(VERSION 3.25.1)
project(omni_ext)

if(NOT DEFINED ENV{OMNI_EXT_ALREADY_INCLUDED})
        set(ENV{OMNI_EXT_ALREADY_INCLUDED} TRUE)

        list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

        include(CPM)
        include(CTest)
        include(CheckSymbolExists)

        enable_testing()

        find_package(PostgreSQL REQUIRED)
find_package(STC REQUIRED)

        function(check_symbols)
                set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
                check_symbol_exists(strverscmp "string.h" HAVE_STRVERSCMP)
                set(CMAKE_REQUIRED_LIBRARIES dl)
                check_symbol_exists(dladdr "dlfcn.h" HAVE_DLADDR)
endfunction()

        check_symbols()

        add_postgresql_extension(
                omni_ext
                SCHEMA omni_ext
                RELOCATABLE false
                SOURCES omni_ext.c control_file.c init.c workers.c strverscmp.c
                DEPENDS_ON libgluepg_stc dynpgext
                UNVERSIONED_SO ON
                SHARED_PRELOAD ON)

        set_property(TARGET omni_ext PROPERTY C_STANDARD 11)

        target_compile_definitions(omni_ext PUBLIC EXT_LIBRARY_NAME=\"$<TARGET_FILE_NAME:omni_ext>\")

        if(HAVE_STRVERSCMP)
                target_compile_definitions(omni_ext PUBLIC HAVE_STRVERSCMP PUBLIC _GNU_SOURCE)
endif()

        if(HAVE_DLADDR)
                target_compile_definitions(omni_ext PUBLIC HAVE_DLADDR PUBLIC _GNU_SOURCE)
endif()

        target_link_libraries(omni_ext libpgaug dynpgext libgluepg_stc)

        add_subdirectory(test)

endif()