#include <metalang99/stmt.h>

#include <assert.h>

int main(void) {

    // ML99_INTRODUCE_VAR_TO_STMT
    {
        if (1)
            ML99_INTRODUCE_VAR_TO_STMT(int x = 5, y = 7) {
                assert(5 == x);
                assert(7 == y);
            }
    }

    // ML99_INTRODUCE_NON_NULL_PTR_TO_STMT
    {
        int x = 5, y = 7;

        // clang-format off
        if (1)
            ML99_INTRODUCE_NON_NULL_PTR_TO_STMT(int, x_ptr, &x)
                ML99_INTRODUCE_NON_NULL_PTR_TO_STMT(int, y_ptr, &y) {
                    assert(x == *x_ptr);
                    assert(y == *y_ptr);
                }
        // clang-format on
    }

    // ML99_CHAIN_EXPR_STMT
    {
        int x, y;

        // clang-format off
        if (1)
            ML99_CHAIN_EXPR_STMT(x = 1)
                ML99_CHAIN_EXPR_STMT(y = 2) {
                    assert(1 == x);
                    assert(2 == y);
                }
        // clang-format on

        // Test -Wunused suppression via ML99_CHAIN_EXPR_STMT.
        int z;

        if (1)
            ML99_CHAIN_EXPR_STMT((void)z);
    }

    // ML99_CHAIN_EXPR_STMT_AFTER
    {
        int x = 5, y = 7;

        if (1) {
            assert(5 == x);
            assert(7 == y);

            ML99_CHAIN_EXPR_STMT_AFTER(x = 1) {
                assert(5 == x);
                assert(7 == y);

                ML99_CHAIN_EXPR_STMT_AFTER(y = 2) {
                    assert(5 == x);
                    assert(7 == y);
                }

                assert(5 == x);
                assert(2 == y);
            }

            assert(1 == x);
            assert(2 == y);
        }
    }

    // ML99_SUPPRESS_UNUSED_BEFORE_STMT
    {
        int x, y;

        // clang-format off
        if (1)
            ML99_SUPPRESS_UNUSED_BEFORE_STMT(x)
                ML99_SUPPRESS_UNUSED_BEFORE_STMT(y)
                    ;
        // clang-format on
    }

    // Clang-Format breaks with the following sequence of statements so they are put into a macro.
    // clang-format off

#define STMT_CHAINING \
    ML99_INTRODUCE_VAR_TO_STMT(int x = 5) \
        ML99_INTRODUCE_NON_NULL_PTR_TO_STMT(int, x_ptr, &x) { \
            assert(x == *x_ptr); \
 \
            ML99_CHAIN_EXPR_STMT(x = 7) \
                ML99_INTRODUCE_VAR_TO_STMT(int y = 5) \
                    ML99_SUPPRESS_UNUSED_BEFORE_STMT(y) { \
                        ML99_CHAIN_EXPR_STMT_AFTER(x = 123) { \
                            assert(7 == x); \
                        } \
 \
                        assert(123 == x); \
                    } \
        }

    // clang-format on

    { STMT_CHAINING }

#undef STMT_CHAINING

    return 0;
}
