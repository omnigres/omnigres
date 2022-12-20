include(CPM)
include(OpenSSL)

find_package(OpenSSL REQUIRED)
CPMAddPackage(URL "https://github.com/curl/curl/releases/download/curl-7_87_0/curl-7.87.0.tar.bz2"
    OPTIONS "BUILD_CURL_EXE OFF BUILD_SHARED_LIBS OFF CURL_USE_LIBSSH2 OFF CURL_ZLIB OFF CURL_DISABLE_LDAP ON OPENSSL_USE_STATIC_LIBS ON")