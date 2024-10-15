#include <metalang99/assert.h>
#include <metalang99/bool.h>

int main(void) {

    // ML99_true, ML99_false
    {
        ML99_ASSERT_EQ(ML99_true(), v(1));
        ML99_ASSERT_EQ(ML99_true(~, ~, ~), v(1));

        ML99_ASSERT_EQ(ML99_false(), v(0));
        ML99_ASSERT_EQ(ML99_false(~, ~, ~), v(0));
    }

    // ML99_not
    {
        ML99_ASSERT_EQ(ML99_not(v(0)), v(1));
        ML99_ASSERT_EQ(ML99_not(v(1)), v(0));
    }

    // ML99_and
    {
        ML99_ASSERT_EQ(ML99_and(v(0), v(0)), v(0));
        ML99_ASSERT_EQ(ML99_and(v(0), v(1)), v(0));
        ML99_ASSERT_EQ(ML99_and(v(1), v(0)), v(0));
        ML99_ASSERT_EQ(ML99_and(v(1), v(1)), v(1));
    }

    // ML99_or
    {
        ML99_ASSERT_EQ(ML99_or(v(0), v(0)), v(0));
        ML99_ASSERT_EQ(ML99_or(v(0), v(1)), v(1));
        ML99_ASSERT_EQ(ML99_or(v(1), v(0)), v(1));
        ML99_ASSERT_EQ(ML99_or(v(1), v(1)), v(1));
    }

    // ML99_xor
    {
        ML99_ASSERT_EQ(ML99_xor(v(0), v(0)), v(0));
        ML99_ASSERT_EQ(ML99_xor(v(0), v(1)), v(1));
        ML99_ASSERT_EQ(ML99_xor(v(1), v(0)), v(1));
        ML99_ASSERT_EQ(ML99_xor(v(1), v(1)), v(0));
    }

    // ML99_boolEq
    {
        ML99_ASSERT_EQ(ML99_boolEq(v(0), v(0)), v(1));
        ML99_ASSERT_EQ(ML99_boolEq(v(0), v(1)), v(0));
        ML99_ASSERT_EQ(ML99_boolEq(v(1), v(0)), v(0));
        ML99_ASSERT_EQ(ML99_boolEq(v(1), v(1)), v(1));
    }

#define MATCH_1_IMPL(...) v(12) // `...` due to the TCC's bug.
#define MATCH_0_IMPL(...) v(9)

    // ML99_boolMatch
    {
        ML99_ASSERT_EQ(ML99_boolMatch(v(1), v(MATCH_)), v(12));
        ML99_ASSERT_EQ(ML99_boolMatch(v(0), v(MATCH_)), v(9));
    }

#undef MATCH_1_IMPL
#undef MATCH_0_IMPL

#define MATCH_1_IMPL(x, y, z) v(ML99_ASSERT_UNEVAL(x == 1 && y == 2 && z == 3))
#define MATCH_0_IMPL(x, y, z) v(ML99_ASSERT_UNEVAL(x == 1 && y == 2 && z == 3))

    // ML99_boolMatchWithArgs
    {
        ML99_EVAL(ML99_boolMatchWithArgs(v(1), v(MATCH_), v(1, 2, 3)));
        ML99_EVAL(ML99_boolMatchWithArgs(v(0), v(MATCH_), v(1, 2, 3)));
    }

#undef MATCH_1_IMPL
#undef MATCH_0_IMPL

    // ML99_if
    {
        ML99_ASSERT_EQ(ML99_if(ML99_true(), v(24), v(848)), v(24));
        ML99_ASSERT_EQ(ML99_if(ML99_true(), v(1549), v(1678)), v(1549));

        ML99_ASSERT_EQ(ML99_if(ML99_false(), v(516), v(115)), v(115));
        ML99_ASSERT_EQ(ML99_if(ML99_false(), v(10), v(6)), v(6));
    }

#define CHECK(...)         CHECK_AUX(__VA_ARGS__)
#define CHECK_AUX(a, b, c) ML99_ASSERT_UNEVAL(a == 1 && b == 2 && c == 3)

#define X 1, 2, 3

    // ML99_IF
    {
        ML99_ASSERT_UNEVAL(ML99_IF(ML99_TRUE(), 24, 848) == 24);
        ML99_ASSERT_UNEVAL(ML99_IF(ML99_FALSE(), 516, 115) == 115);

        // Ensure that a branch can expand to multiple commas (`X`).
        CHECK(ML99_IF(ML99_TRUE(), X, ~));
        CHECK(ML99_IF(ML99_FALSE(), ~, X));
    }

#undef CHECK
#undef CHECK_AUX
#undef X

    // Plain macros
    {
        ML99_ASSERT_UNEVAL(ML99_TRUE());
        ML99_ASSERT_UNEVAL(ML99_TRUE(~, ~, ~));

        ML99_ASSERT_UNEVAL(!ML99_FALSE());
        ML99_ASSERT_UNEVAL(!ML99_FALSE(~, ~, ~));

        ML99_ASSERT_UNEVAL(ML99_NOT(0) == 1);
        ML99_ASSERT_UNEVAL(ML99_NOT(1) == 0);

        ML99_ASSERT_UNEVAL(ML99_AND(0, 1) == 0);
        ML99_ASSERT_UNEVAL(ML99_OR(0, 1) == 1);
        ML99_ASSERT_UNEVAL(ML99_XOR(0, 1) == 1);
        ML99_ASSERT_UNEVAL(ML99_BOOL_EQ(0, 1) == 0);
    }
}
