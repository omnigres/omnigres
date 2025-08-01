if(NOT (CMAKE_CROSSCOMPILING AND MSVC))
    # When using MSVC the cmake build via ExternalProject seems to inherit the target architecture,
    # which breaks the test. Since we practically don't care about this, and we just want to ensure
    # that installing an executable works, skipping this test when cross-compiling is fine.
    corrosion_tests_add_test(install_rust_bin "generated_from_installed_bin")

    set_tests_properties("install_rust_bin_run_generated_from_installed_bin"
                         PROPERTIES PASS_REGULAR_EXPRESSION
                         "Hello World! I'm generated code"
    )
endif()

# Todo: Fix and re-enable tests on Windows
if(NOT CMAKE_CROSSCOMPILING AND NOT WIN32)
    corrosion_tests_add_test(install_lib "main-static;main-shared")

    set_tests_properties("install_lib_run_main-static" "install_lib_run_main-shared"
                         PROPERTIES PASS_REGULAR_EXPRESSION
                         "The sum is 11"
    )
endif()

# Further tests we should add:
# - Test installing a Rust executable, that requires a (C/C++) shared library at runtime.
#   Note: We should delete the build directory of the subproject
#         before running the installed rust executable, to insure the shared library is loaded from the
#         installed location and not from the build dir.