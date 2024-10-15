#include <metalang99/assert.h>
#include <metalang99/either.h>
#include <metalang99/nat.h>

int main(void) {

#define MATCH_IMPL(either)  ML99_match(v(either), v(MATCH_))
#define MATCH_left_IMPL(x)  v(ML99_ASSERT_UNEVAL(x == 18))
#define MATCH_right_IMPL(x) v(ML99_ASSERT_UNEVAL(x == 4))

    // Pattern matching
    {
        ML99_EVAL(ML99_call(MATCH, ML99_left(v(18))));
        ML99_EVAL(ML99_call(MATCH, ML99_right(v(4))));
    }

#undef MATCH_IMPL
#undef MATCH_left_IMPL
#undef MATCH_right_IMPL

#define VAL v(abc ? +-148 % "hello world")

    // ML99_isLeft
    {
        ML99_ASSERT(ML99_isLeft(ML99_left(VAL)));
        ML99_ASSERT(ML99_not(ML99_isLeft(ML99_right(VAL))));
    }

    // ML99_IS_LEFT
    {
        ML99_ASSERT_UNEVAL(ML99_IS_LEFT(ML99_LEFT(VAL)));
        ML99_ASSERT_UNEVAL(!ML99_IS_LEFT(ML99_RIGHT(VAL)));
    }

    // ML99_isRight
    {
        ML99_ASSERT(ML99_isRight(ML99_right(VAL)));
        ML99_ASSERT(ML99_not(ML99_isRight(ML99_left(VAL))));
    }

    // ML99_IS_RIGHT
    {
        ML99_ASSERT_UNEVAL(ML99_IS_RIGHT(ML99_RIGHT(VAL)));
        ML99_ASSERT_UNEVAL(!ML99_IS_RIGHT(ML99_LEFT(VAL)));
    }

    // ML99_eitherEq
    {
        ML99_ASSERT(ML99_eitherEq(v(ML99_natEq), ML99_left(v(123)), ML99_left(v(123))));
        ML99_ASSERT(ML99_not(ML99_eitherEq(v(ML99_natEq), ML99_left(v(18)), ML99_left(v(123)))));

        ML99_ASSERT(ML99_eitherEq(v(ML99_natEq), ML99_right(v(123)), ML99_right(v(123))));
        ML99_ASSERT(ML99_not(ML99_eitherEq(v(ML99_natEq), ML99_right(v(18)), ML99_right(v(123)))));

        ML99_ASSERT(ML99_not(ML99_eitherEq(v(ML99_natEq), ML99_left(v(123)), ML99_right(v(123)))));
        ML99_ASSERT(ML99_not(ML99_eitherEq(v(ML99_natEq), ML99_left(v(123)), ML99_right(v(4)))));
        ML99_ASSERT(ML99_not(ML99_eitherEq(v(ML99_natEq), ML99_right(v(123)), ML99_left(v(123)))));
        ML99_ASSERT(ML99_not(ML99_eitherEq(v(ML99_natEq), ML99_right(v(123)), ML99_left(v(4)))));
    }

    // ML99_unwrapLeft
    { ML99_ASSERT_EQ(ML99_unwrapLeft(ML99_left(v(123))), v(123)); }

    // ML99_unwrapRight
    { ML99_ASSERT_EQ(ML99_unwrapRight(ML99_right(v(123))), v(123)); }
}
