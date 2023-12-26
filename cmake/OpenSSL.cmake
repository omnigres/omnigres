include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)
if(NOT DEFINED OPENSSL_CONFIGURED)

    set(OPENSSL_USE_STATIC_LIBS ON CACHE INTERNAL "OpenSSL")
    find_package(OpenSSL)

    if(NOT OPENSSL_FOUND OR NOT OPENSSL_VERSION VERSION_EQUAL "3.2")


        find_package(Perl)
        if(PERL_FOUND AND PERL_VERSION_STRING VERSION_GREATER_EQUAL "5.10") # Version requirement from NOTES-PERL.md
            message(STATUS "Using Perl at ${PERL_EXECUTABLE}")
            find_program(CPAN_EXECUTABLE NAMES cpan REQUIRED)
            message(STATUS "Using CPAN at ${CPAN_EXECUTABLE}")
        else()
            # Required to configure OpenSSL
            CPMAddPackage(NAME perl GITHUB_REPOSITORY Perl/perl5 VERSION 5.38.2 DOWNLOAD_ONLY YES)

            if(NOT OPENSSL_PERL_CONFIGURED)
                if(perl_ADDED)
                    execute_process(COMMAND ./Configure -des -Dprefix=${perl_BINARY_DIR} WORKING_DIRECTORY ${perl_SOURCE_DIR} RESULT_VARIABLE CONFIGURE_RC)
                    if(NOT CONFIGURE_RC EQUAL 0)
                        message(FATAL_ERROR "Failed to configure Perl")
                    endif()
                    # We don't use -j in make below as it seems to routinely run out of resources when done
                    # this way
                    execute_process(COMMAND make all WORKING_DIRECTORY ${perl_SOURCE_DIR} RESULT_VARIABLE BUILD_RC)
                    if(NOT BUILD_RC EQUAL 0)
                        message(FATAL_ERROR "Failed to build Perl")
                    endif()
                    execute_process(COMMAND make install WORKING_DIRECTORY ${perl_SOURCE_DIR} RESULT_VARIABLE INSTALL_RC)
                    if(NOT BUILD_RC EQUAL 0)
                        message(FATAL_ERROR "Failed to install Perl")
                    endif()
                else()
                    message(FATAL_ERROR "Can't fetch Perl")
                endif()
                set(OPENSSL_PERL_CONFIGURED TRUE CACHE INTERNAL "OpenSSL")
            endif()
            find_program(PERL_EXECUTABLE NAMES perl PATHS ${perl_BINARY_DIR}/bin REQUIRED NO_DEFAULT_PATH)
            find_program(CPAN_EXECUTABLE NAMES cpan PATHS ${perl_BINARY_DIR}/bin REQUIRED NO_DEFAULT_PATH)
        endif()

        CPMAddPackage(NAME openssl GITHUB_REPOSITORY openssl/openssl VERSION 3.2.0 GIT_TAG openssl-3.2.0 DOWNLOAD_ONLY YES)

        if(openssl_ADDED)
            # NOTE: it should pick up this dependency from `external/perl` in OpenSSL source dir
            #            execute_process(COMMAND ${CPAN_EXECUTABLE} -i Text::Template RESULT_VARIABLE CPAN_RC)
            #            if(NOT CPAN_RC EQUAL 0)
            #                message(FATAL_ERROR "Can't install Text::Template")
            #            endif()
            execute_process(COMMAND ${PERL_EXECUTABLE} ${openssl_SOURCE_DIR}/Configure --prefix=${openssl_BINARY_DIR}
                    WORKING_DIRECTORY ${openssl_SOURCE_DIR} RESULT_VARIABLE CONFIGURE_RC)
            if(NOT CONFIGURE_RC EQUAL 0)
                message(FATAL_ERROR "Can't configure OpenSSL")
            endif()
            execute_process(COMMAND make -j ${CMAKE_BUILD_PARALLEL_LEVEL} all WORKING_DIRECTORY ${openssl_SOURCE_DIR} RESULT_VARIABLE BUILD_RC)
            if(NOT BUILD_RC EQUAL 0)
                message(FATAL_ERROR "Failed to build OpenSSL")
            endif()
            execute_process(COMMAND make -j ${CMAKE_BUILD_PARALLEL_LEVEL} install WORKING_DIRECTORY ${openssl_SOURCE_DIR} RESULT_VARIABLE INSTALL_RC)
            if(NOT BUILD_RC EQUAL 0)
                message(FATAL_ERROR "Failed to install OpenSSL")
            endif()
        else()
            message(FATAL_ERROR "Can't fetch OpenSSL")
        endif()

        set(OPENSSL_ROOT_DIR ${openssl_BINARY_DIR} CACHE INTERNAL "OpenSSL")
    else()
        file(REAL_PATH "${OPENSSL_INCLUDE_DIR}/.." _OPENSSL_ROOT_DIR)
        set(OPENSSL_ROOT_DIR ${_OPENSSL_ROOT_DIR} CACHE INTERNAL "OpenSSL")
    endif()

    set(OPENSSL_CONFIGURED TRUE CACHE INTERNAL "OpenSSL")

endif()