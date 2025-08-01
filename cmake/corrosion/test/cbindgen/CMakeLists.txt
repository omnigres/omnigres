corrosion_tests_add_test(cbindgen_rust2cpp_auto "cpp-exe" TEST_SRC_DIR auto)
corrosion_tests_add_test(cbindgen_manual "cpp-exe" TEST_SRC_DIR manual)

set_tests_properties(cbindgen_rust2cpp_auto_run_cpp-exe cbindgen_manual_run_cpp-exe
         PROPERTIES PASS_REGULAR_EXPRESSION
        "^add_point Result: Point { x: 100, y: 100 }\r?\n$"
)

add_test(NAME "cbindgen_install_configure"
         COMMAND
         ${CMAKE_COMMAND}
         -S "${CMAKE_CURRENT_SOURCE_DIR}/install_lib"
         -B "${CMAKE_CURRENT_BINARY_DIR}/build_install_lib"
         "-G${CMAKE_GENERATOR}"
            --install-prefix "${CMAKE_CURRENT_BINARY_DIR}/build_install_lib/test_install_lib_install_dir"

         COMMAND_EXPAND_LISTS
)

add_test(NAME "cbindgen_install"
         COMMAND
         ${CMAKE_COMMAND}
         --build "${CMAKE_CURRENT_BINARY_DIR}/build_install_lib"
         --target install
         --config Debug
)

add_test(NAME cbindgen_install_check_header_installed
         COMMAND
         "${CMAKE_COMMAND}"
         -P "${CMAKE_CURRENT_SOURCE_DIR}/../TestFileExists.cmake"
         "${CMAKE_CURRENT_BINARY_DIR}/build_install_lib/test_install_lib_install_dir/include/rust-lib.h"
)

add_test(NAME cbindgen_install_clean
         COMMAND
         "${CMAKE_COMMAND}"
            -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/build_install_lib/"
)
set_tests_properties("cbindgen_install_configure" PROPERTIES FIXTURES_SETUP "configure_fixture_cbindgen_install")
set_tests_properties("cbindgen_install" PROPERTIES FIXTURES_REQUIRED "configure_fixture_cbindgen_install")

set_tests_properties("cbindgen_install" PROPERTIES FIXTURES_SETUP "install_fixture_cbindgen_install")
set_tests_properties("cbindgen_install_check_header_installed" PROPERTIES FIXTURES_REQUIRED "install_fixture_cbindgen_install")
set_tests_properties("cbindgen_install_check_header_installed" PROPERTIES FIXTURES_SETUP "fx_check_header_installed")
set_tests_properties(cbindgen_install_clean PROPERTIES FIXTURES_CLEANUP "configure_fixture_cbindgen_install;install_fixture_cbindgen_install;fx_check_header_installed")

# Todo: We also should add a cpp2rust test with the following setup:
# - A rust lib that is used by a rust executable
# - cbindgen creates bindings for the rust-lib
# - c++ code uses the rust lib and is used in turn by the rust bin.

# todo: add a test for the DEPFILE and correct regenerating if the sources are touched.
