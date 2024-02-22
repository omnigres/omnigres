cmake_minimum_required(VERSION 3.25.1)
project(logtalk_skeleton)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

include(CPM)
include(SWIprolog)
include(Logtalk)

find_swipl(SWIPL)
find_logtalk(Logtalk ${SWIPL_VERSION})

add_executable(logtalk_skeleton main.c)

target_link_swipl(logtalk_skeleton SWIPL)

logtalk_qlf(Logtalk logtalk_skeleton SOURCES main.lgt LOADER loader.lgt)
