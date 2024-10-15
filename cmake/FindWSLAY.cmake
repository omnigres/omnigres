include(CPM)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/versions.cmake)
CPMAddPackage(NAME wslay SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../deps/wslay VERSION ${VERSION_wslay}
        OPTIONS "WSLAY_CONFIGURE_INSTALL OFF")

set(WSLAY_INCLUDE_DIRS "${wslay_SOURCE_DIR}/lib/include")
set(WSLAY_INCLUDE_DIR "${wslay_SOURCE_DIR}/lib/include")
set(WSLAY_LIBRARY_DIRS "${wslay_BINARY_DIR}/lib")
set(WSLAY_LIBRARIES "wslay")
set(WSLAY_FOUND ON)
