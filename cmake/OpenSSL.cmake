if(APPLE)
    # TODO: reassess pinning to 3.0 as this is a temporary workaround for this issue:
    # https://github.com/openssl/openssl/issues/20753

    execute_process(COMMAND brew --prefix openssl@3.0
        OUTPUT_VARIABLE OPENSSL_PREIX RESULT_VARIABLE OPENSSL_RC
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(NOT OPENSSL_RC EQUAL 0)
        message(FATAL_ERROR "No OpenSSL found, use homebrew to install one")
    endif()

    message(STATUS "Found OpenSSL at ${OPENSSL_PREIX}")
    set(OPENSSL_ROOT_DIR ${OPENSSL_PREIX} CACHE INTERNAL "OpenSSL")
elseif(UNIX)
    find_package(PkgConfig)
    pkg_check_modules(_OPENSSL openssl)
endif()

set(OPENSSL_USE_STATIC_LIBS ON)
