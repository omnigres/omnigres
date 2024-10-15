cmake_minimum_required(VERSION 3.25.1)
project(omni_xml)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

include(CPM)

enable_testing()

include(../../cmake/dependencies/versions.cmake)
CPMAddPackage(NAME pugixml SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../deps/pugixml VERSION ${VERSION_pugixml} OPTIONS "PUGIXML_NO_EXCEPTIONS ON")

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_xml
        SCHEMA omni_xml
        RELOCATABLE false
        SOURCES omni_xml.cc)

set_property(TARGET omni_xml PROPERTY CXX_STANDARD 11)
target_link_libraries(omni_xml pugixml::static)
