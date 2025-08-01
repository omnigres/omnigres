cmake_minimum_required(VERSION 3.22)
project(test_rust_bin VERSION 0.1.0)
include(../../../test_header.cmake)
include(GNUInstallDirs)

corrosion_import_crate(MANIFEST_PATH Cargo.toml)
corrosion_install(TARGETS my_rust_bin)
