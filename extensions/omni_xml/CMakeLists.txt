cmake_minimum_required(VERSION 3.25.1)
project(omni_xml)

include(CTest)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

include(CPM)
include(FindPkgConfig)

enable_testing()

pkg_check_modules (pugixml pugixml)
if(NOT pugixml_FOUND)
  CPMAddPackage(NAME pugixml GIT_REPOSITORY https://github.com/zeux/pugixml VERSION 1.14 OPTIONS "PUGIXML_NO_EXCEPTIONS ON")
endif()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_xml
        SCHEMA omni_xml
        RELOCATABLE false
        SOURCES omni_xml.cc)

set_property(TARGET omni_xml PROPERTY CXX_STANDARD 11)
target_link_libraries(omni_xml pugixml::static)
