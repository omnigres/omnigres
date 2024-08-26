include(CPM)

CPMAddPackage(NAME wslay GIT_REPOSITORY https://github.com/tatsuhiro-t/wslay GIT_TAG 0e7d106 VERSION 0e7d106
        OPTIONS "WSLAY_CONFIGURE_INSTALL OFF")

set(WSLAY_INCLUDE_DIRS "${wslay_SOURCE_DIR}/lib/include")
set(WSLAY_INCLUDE_DIR "${wslay_SOURCE_DIR}/lib/include")
set(WSLAY_LIBRARY_DIRS "${wslay_BINARY_DIR}/lib")
set(WSLAY_LIBRARIES "wslay")
set(WSLAY_FOUND ON)
