cmake_minimum_required(VERSION 3.25.1)
project(omni_txn)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)

include(CPM)
include(CTest)

enable_testing()

find_package(PostgreSQL REQUIRED)

add_postgresql_extension(
        omni_txn
        VERSION 0.5.0
        COMMENT "Transaction management"
        SCHEMA omni_txn
        RELOCATABLE false
        SOURCES omni_txn.c retry.c linearization.c
        DEPENDS_ON omni libomni
        TESTS_REQUIRE omni_edbg
)

target_link_libraries(omni_txn libomni)