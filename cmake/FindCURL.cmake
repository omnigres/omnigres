include(CPM)
include(OpenSSL)

find_package(OpenSSL REQUIRED)

set(BUILD_SHARED_LIBS OFF)
set(CURL_USE_LIBSSH2 OFF)
set(CURL_ZLIB OFF)
set(CURL_DISABLE_LDAP ON)
CPMAddPackage(URL "https://github.com/curl/curl/releases/download/curl-7_87_0/curl-7.87.0.tar.bz2")
set_property(TARGET libcurl PROPERTY POSITION_INDEPENDENT_CODE ON)