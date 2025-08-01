corrosion_tests_add_test(find_rust "")
corrosion_tests_add_test(rustup_proxy "")

find_program(rustup rustup)
if(NOT rustup)
    set_tests_properties(rustup_proxy_build rustup_proxy_cleanup PROPERTIES DISABLED 1)
endif()
