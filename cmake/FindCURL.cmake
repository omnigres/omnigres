list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

include(CPM)
include(OpenSSL)

find_package(OpenSSL REQUIRED)

CPMAddPackage(URL "https://github.com/curl/curl/releases/download/curl-7_87_0/curl-7.87.0.tar.bz2"
    OPTIONS "BUILD_SHARED_LIBS OFF" "CURL_USE_LIBSSH2 OFF" "CURL_ZLIB OFF" "CURL_DISABLE_LDAP ON" "BULD_CURL_EXE OFF")
set_property(TARGET libcurl PROPERTY POSITION_INDEPENDENT_CODE ON)