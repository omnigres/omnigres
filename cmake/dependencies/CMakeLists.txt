cmake_minimum_required(VERSION 3.25.1)
project(omnigres_dependencies)

include(../CPM.cmake)
include(versions.cmake)

CPMAddPackage(NAME h2o GIT_REPOSITORY https://github.com/h2o/h2o GIT_TAG ${GIT_TAG_h2o} VERSION ${VERSION_h2o} DOWNLOAD_ONLY YES)
CPMAddPackage(NAME pugixml GIT_REPOSITORY https://github.com/zeux/pugixml VERSION ${VERSION_pugixml} DOWNLOAD_ONLY YES)
CPMAddPackage(NAME metalang99 GIT_REPOSITORY https://github.com/Hirrolot/metalang99 VERSION ${VERSION_metalang} DOWNLOAD_ONLY YES)
CPMAddPackage(NAME stc GIT_REPOSITORY https://github.com/stclib/STC GIT_TAG ${GIT_TAG_stc} VERSION ${VERSION_stc} DOWNLOAD_ONLY YES)
CPMAddPackage(NAME uriparser GIT_REPOSITORY https://github.com/uriparser/uriparser GIT_TAG ${GIT_TAG_uriparser} VERSION ${VERSION_uriparser} DOWNLOAD_ONLY YES)
CPMAddPackage(NAME wslay GIT_REPOSITORY https://github.com/tatsuhiro-t/wslay GIT_TAG ${GIT_TAG_wslay} VERSION ${VERSION_wslay} DOWNLOAD_ONLY YES)
CPMAddPackage(NAME libfyaml GITHUB_REPOSITORY "yrashk/libfyaml" GIT_TAG ${GIT_TAG_libfyaml} VERSION ${VERSION_libfyaml} DOWNLOAD_ONLY YES)
