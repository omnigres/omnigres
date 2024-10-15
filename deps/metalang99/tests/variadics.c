#include <metalang99/assert.h>
#include <metalang99/variadics.h>

int main(void) {

    // ML99_variadicsGet
    {
        ML99_ASSERT_EMPTY(ML99_variadicsGet(0)(v()));
        ML99_ASSERT_EQ(ML99_variadicsGet(0)(v(19)), v(19));
        ML99_ASSERT_EQ(ML99_variadicsGet(0)(v(19, 8)), v(19));
        ML99_ASSERT_EQ(ML99_variadicsGet(0)(v(19, 8, 7378)), v(19));

        ML99_ASSERT_EQ(ML99_variadicsGet(1)(v(19, 8)), v(8));
        ML99_ASSERT_EQ(ML99_variadicsGet(2)(v(19, 8, 7378)), v(7378));
        ML99_ASSERT_EQ(ML99_variadicsGet(3)(v(19, 8, 7378, 10)), v(10));
        ML99_ASSERT_EQ(ML99_variadicsGet(4)(v(19, 8, 7378, 10, 121)), v(121));
        ML99_ASSERT_EQ(ML99_variadicsGet(5)(v(19, 8, 7378, 10, 121, 1)), v(1));
        ML99_ASSERT_EQ(ML99_variadicsGet(6)(v(19, 8, 7378, 10, 121, 1, 80)), v(80));
        ML99_ASSERT_EQ(ML99_variadicsGet(7)(v(19, 8, 7378, 10, 121, 1, 80, 23)), v(23));
    }

    // ML99_VARIADICS_GET
    {
        ML99_ASSERT_EMPTY_UNEVAL(ML99_VARIADICS_GET(0)());

        ML99_ASSERT_UNEVAL(ML99_VARIADICS_GET(0)(19) == 19);
        ML99_ASSERT_UNEVAL(ML99_VARIADICS_GET(0)(19, 8) == 19);
        ML99_ASSERT_UNEVAL(ML99_VARIADICS_GET(0)(19, 8, 7378) == 19);

        ML99_ASSERT_UNEVAL(ML99_VARIADICS_GET(1)(19, 8) == 8);
        ML99_ASSERT_UNEVAL(ML99_VARIADICS_GET(1)(19, 8, 7378) == 8);
    }

#define CHECK_TAIL(...)         CHECK_TAIL_AUX(__VA_ARGS__)
#define CHECK_TAIL_AUX(a, b, c) ML99_ASSERT_UNEVAL(a == 51 && b == 3 && c == 9)

    // ML99_variadicsTail
    { CHECK_TAIL(ML99_EVAL(ML99_variadicsTail(v(9191, 51, 3, 9)))); }

    // ML99_VARIADICS_TAIL
    { CHECK_TAIL(ML99_VARIADICS_TAIL(9191, 51, 3, 9)); }

#undef CHECK_TAIL
#undef CHECK_TAIL_AUX

#define _5_ARGS  v(~, ~, ~, ~, ~)
#define _10_ARGS _5_ARGS, _5_ARGS
#define _50_ARGS _10_ARGS, _10_ARGS, _10_ARGS, _10_ARGS, _10_ARGS

    // ML99_variadicsCount
    {
        ML99_ASSERT_EQ(ML99_variadicsCount(v()), v(1));
        ML99_ASSERT_EQ(ML99_variadicsCount(v(~)), v(1));
        ML99_ASSERT_EQ(ML99_variadicsCount(v(~, ~)), v(2));
        ML99_ASSERT_EQ(ML99_variadicsCount(v(~, ~, ~)), v(3));
        ML99_ASSERT_EQ(ML99_variadicsCount(v(~, ~, ~, ~)), v(4));
        ML99_ASSERT_EQ(ML99_variadicsCount(_5_ARGS), v(5));
        ML99_ASSERT_EQ(ML99_variadicsCount(_5_ARGS, v(~)), v(6));
        ML99_ASSERT_EQ(ML99_variadicsCount(_5_ARGS, v(~, ~)), v(7));
        ML99_ASSERT_EQ(ML99_variadicsCount(_5_ARGS, v(~, ~, ~)), v(8));
        ML99_ASSERT_EQ(ML99_variadicsCount(_5_ARGS, v(~, ~, ~, ~)), v(9));
        ML99_ASSERT_EQ(ML99_variadicsCount(_10_ARGS), v(10));
        ML99_ASSERT_EQ(ML99_variadicsCount(_10_ARGS, v(~)), v(11));
        ML99_ASSERT_EQ(ML99_variadicsCount(_50_ARGS, _10_ARGS, v(~, ~, ~)), v(63));
    }

#define X(...)    ML99_OVERLOAD(X_, __VA_ARGS__)
#define X_1(a)    ML99_ASSERT_UNEVAL(a == 123)
#define X_2(a, b) ML99_ASSERT_UNEVAL(a == 93145 && b == 456)
#define X_7(a, b, c, d, e, f, g)                                                                   \
    ML99_ASSERT_UNEVAL(a == 1516 && b == 1 && c == 9 && d == 111 && e == 119 && f == 677 && g == 62)

    // ML99_OVERLOAD
    {
        X(123);
        X(93145, 456);
        X(1516, 1, 9, 111, 119, 677, 62);
    }

#undef X_IMPL
#undef X_1_IMPL
#undef X_2_IMPL
#undef X_7_IMPL

    // ML99_VARIADICS_COUNT
    {
        ML99_ASSERT_EQ(v(ML99_VARIADICS_COUNT()), v(1));
        ML99_ASSERT_EQ(v(ML99_VARIADICS_COUNT(~)), v(1));
        ML99_ASSERT_EQ(v(ML99_VARIADICS_COUNT(~, ~)), v(2));
        ML99_ASSERT_EQ(v(ML99_VARIADICS_COUNT(~, ~, ~)), v(3));
    }

#undef _5_ARGS
#undef _10_ARGS
#undef _50_ARGS
#undef _100_ARGS

    // ML99_variadicsIsSingle
    {
        ML99_ASSERT(ML99_variadicsIsSingle(v()));
        ML99_ASSERT(ML99_variadicsIsSingle(v(~)));
        ML99_ASSERT(ML99_not(ML99_variadicsIsSingle(v(~, ~, ~))));
    }

    // ML99_VARIADICS_IS_SINGLE
    {
        ML99_ASSERT_UNEVAL(ML99_VARIADICS_IS_SINGLE());
        ML99_ASSERT_UNEVAL(ML99_VARIADICS_IS_SINGLE(~));
        ML99_ASSERT_UNEVAL(ML99_NOT(ML99_VARIADICS_IS_SINGLE(~, ~, ~)));
    }

#define CHECK_EXPAND(...) CHECK(__VA_ARGS__)

#define CHECK(_, x, y, z) ML99_ASSERT_UNEVAL(x == 1987 && y == 2987 && z == 3987)
#define F_IMPL(x)         v(, x##987)
#define F_ARITY           1

    // ML99_variadicsForEach
    { CHECK_EXPAND(ML99_EVAL(ML99_variadicsForEach(v(F), v(1, 2, 3)))); }

#undef CHECK
#undef F_IMPL
#undef F_ARITY

#define CHECK(_, x, y, z) ML99_ASSERT_UNEVAL(x == 10 && y == 21 && z == 32)
#define F_IMPL(x, i)      v(, ), v(x##i)
#define F_ARITY           2

    // ML99_variadicsForEachI
    { CHECK_EXPAND(ML99_EVAL(ML99_variadicsForEachI(v(F), v(1, 2, 3)))); }

#undef CHECK
#undef F_IMPL
#undef F_ARITY

#undef CHECK_EXPAND
}
