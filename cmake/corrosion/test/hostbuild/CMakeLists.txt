corrosion_tests_add_test(hostbuild "rust-host-program" IS_HOSTBUILD)

set_tests_properties("hostbuild_run_rust-host-program" PROPERTIES PASS_REGULAR_EXPRESSION
        "^ok\r?\nHello Rust Hostbuild, I am an external C function"
        )

