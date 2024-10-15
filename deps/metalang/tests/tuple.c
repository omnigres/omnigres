#include <metalang99/assert.h>
#include <metalang99/tuple.h>

int main(void) {

#define CHECK(x, y)        ML99_ASSERT_UNEVAL(x == 518 && y == 1910)
#define CHECK_EXPAND(args) CHECK args

    // ML99_tuple
    { CHECK_EXPAND(ML99_EVAL(ML99_tuple(v(518, 1910)))); }

    // ML99_TUPLE
    { CHECK_EXPAND(ML99_TUPLE(518, 1910)); }

#undef CHECK
#undef CHECK_EXPAND

    // ML99_untupleEval
    { ML99_ASSERT_EQ(ML99_untupleEval(v((v(198)))), v(198)); }

    // ML99_untuple(Checked)
    {
        ML99_ASSERT_EQ(ML99_untuple(v((198))), v(198));
        ML99_ASSERT_EQ(ML99_untupleChecked(v((198))), v(198));
    }

    // ML99_UNTUPLE
    { ML99_ASSERT_UNEVAL(ML99_UNTUPLE((198)) == 198); }

    // ML99_tupleEval + ML99_untupleEval
    { ML99_ASSERT_EQ(ML99_untupleEval(ML99_tupleEval(v(187))), v(187)); }

    // ML99_tuple + ML99_untuple
    { ML99_ASSERT_EQ(ML99_untuple(ML99_tuple(v(187))), v(187)); }

    // ML99_isTuple
    {
        ML99_ASSERT(ML99_isTuple(v((1, 2, 3))));
        ML99_ASSERT(ML99_not(ML99_isTuple(v(123))));
    }

    // ML99_IS_TUPLE
    {
        ML99_ASSERT_UNEVAL(ML99_IS_TUPLE((1, 2, 3)));
        ML99_ASSERT_UNEVAL(!ML99_IS_TUPLE(123));
    }

    // ML99_isUntuple
    {
        ML99_ASSERT(ML99_not(ML99_isUntuple(v((1, 2, 3)))));

        ML99_ASSERT(ML99_isUntuple(v(123)));
        ML99_ASSERT(ML99_isUntuple(v(123())));
        ML99_ASSERT(ML99_isUntuple(v(123(~))));
        ML99_ASSERT(ML99_isUntuple(v(123(~, ~, ~))));

        ML99_ASSERT(ML99_isUntuple(v(()())));
        ML99_ASSERT(ML99_isUntuple(v(()() ~)));

        ML99_ASSERT(ML99_isUntuple(v((~)(~))));
        ML99_ASSERT(ML99_isUntuple(v((~)(~) ~)));

        ML99_ASSERT(ML99_isUntuple(v((~, ~, ~)(~, ~, ~))));
        ML99_ASSERT(ML99_isUntuple(v((~, ~, ~)(~, ~, ~) ~)));

        ML99_ASSERT(ML99_isUntuple(v((~, ~, ~)(~, ~, ~)(~, ~, ~))));
        ML99_ASSERT(ML99_isUntuple(v((~, ~, ~)(~, ~, ~)(~, ~, ~) ~)));
    }

    // ML99_IS_UNTUPLE
    {
        ML99_ASSERT_UNEVAL(!ML99_IS_UNTUPLE((1, 2, 3)));
        ML99_ASSERT_UNEVAL(ML99_IS_UNTUPLE(123));
        ML99_ASSERT_UNEVAL(ML99_IS_UNTUPLE((~)(~)));
    }

    // ML99_tupleCount
    {
        ML99_ASSERT_EQ(ML99_tupleCount(v((1, 2, 3))), v(3));
        ML99_ASSERT_EQ(ML99_tupleCount(v((*))), v(1));
    }

    // ML99_TUPLE_COUNT
    {
        ML99_ASSERT_UNEVAL(ML99_TUPLE_COUNT((1, 2, 3)) == 3);
        ML99_ASSERT_UNEVAL(ML99_TUPLE_COUNT((*)) == 1);
    }

    // ML99_tupleIsSingle
    {
        ML99_ASSERT(ML99_not(ML99_tupleIsSingle(v((1, 2, 3)))));
        ML99_ASSERT(ML99_tupleIsSingle(v((*))));
    }

    // ML99_TUPLE_IS_SINGLE
    {
        ML99_ASSERT_UNEVAL(!ML99_TUPLE_IS_SINGLE((1, 2, 3)));
        ML99_ASSERT_UNEVAL(ML99_TUPLE_IS_SINGLE((*)));
    }

    // ML99_tupleGet
    {
        ML99_ASSERT_EMPTY(ML99_tupleGet(0)(v(())));
        ML99_ASSERT_EQ(ML99_tupleGet(0)(v((19))), v(19));
        ML99_ASSERT_EQ(ML99_tupleGet(0)(v((19, 8))), v(19));
        ML99_ASSERT_EQ(ML99_tupleGet(0)(v((19, 8, 7378))), v(19));

        ML99_ASSERT_EQ(ML99_tupleGet(1)(v((19, 8))), v(8));
        ML99_ASSERT_EQ(ML99_tupleGet(2)(v((19, 8, 7378))), v(7378));
        ML99_ASSERT_EQ(ML99_tupleGet(3)(v((19, 8, 7378, 10))), v(10));
        ML99_ASSERT_EQ(ML99_tupleGet(4)(v((19, 8, 7378, 10, 121))), v(121));
        ML99_ASSERT_EQ(ML99_tupleGet(5)(v((19, 8, 7378, 10, 121, 1))), v(1));
        ML99_ASSERT_EQ(ML99_tupleGet(6)(v((19, 8, 7378, 10, 121, 1, 80))), v(80));
        ML99_ASSERT_EQ(ML99_tupleGet(7)(v((19, 8, 7378, 10, 121, 1, 80, 23))), v(23));
    }

    // ML99_TUPLE_GET
    {
        ML99_ASSERT_EMPTY_UNEVAL(ML99_TUPLE_GET(0)(()));

        ML99_ASSERT_UNEVAL(ML99_TUPLE_GET(0)((19)) == 19);
        ML99_ASSERT_UNEVAL(ML99_TUPLE_GET(0)((19, 8)) == 19);
        ML99_ASSERT_UNEVAL(ML99_TUPLE_GET(0)((19, 8, 7378)) == 19);
    }

#define CHECK_TAIL(...)         CHECK_TAIL_AUX(__VA_ARGS__)
#define CHECK_TAIL_AUX(a, b, c) ML99_ASSERT_UNEVAL(a == 51 && b == 3 && c == 9)

    // ML99_tupleTail
    { CHECK_TAIL(ML99_EVAL(ML99_tupleTail(v((9191, 51, 3, 9))))); }

    // ML99_TUPLE_TAIL
    { CHECK_TAIL(ML99_TUPLE_TAIL((9191, 51, 3, 9))); }

#undef CHECK_TAIL
#undef CHECK_TAIL_AUX

#define CHECK(a, b, c)     ML99_ASSERT_UNEVAL(a == 1 && b == 2 && c == 3)
#define CHECK_EXPAND(args) CHECK args

    // ML99_tuple(Append|Prepend)
    {
        CHECK_EXPAND(ML99_EVAL(ML99_tupleAppend(ML99_tuple(v(1)), v(2, 3))));
        CHECK_EXPAND(ML99_EVAL(ML99_tuplePrepend(ML99_tuple(v(3)), v(1, 2))));
    }

    // ML99_TUPLE(APPEND|PREPEND)
    {
        CHECK_EXPAND(ML99_TUPLE_APPEND((1), 2, 3));
        CHECK_EXPAND(ML99_TUPLE_PREPEND((3), 1, 2));
    }

#undef CHECK
#undef CHECK_EXPAND

#define CHECK_EXPAND(...) CHECK(__VA_ARGS__)

#define CHECK(_, x, y, z) ML99_ASSERT_UNEVAL(x == 1987 && y == 2987 && z == 3987)
#define F_IMPL(x)         v(, x##987)
#define F_ARITY           1

    // ML99_tupleForEach
    { CHECK_EXPAND(ML99_EVAL(ML99_tupleForEach(v(F), v((1, 2, 3))))); }

#undef CHECK
#undef F_IMPL
#undef F_ARITY

#define CHECK(_, x, y, z) ML99_ASSERT_UNEVAL(x == 10 && y == 21 && z == 32)
#define F_IMPL(x, i)      v(, ), v(x##i)
#define F_ARITY           2

    // ML99_tupleForEachI
    { CHECK_EXPAND(ML99_EVAL(ML99_tupleForEachI(v(F), v((1, 2, 3))))); }

#undef CHECK
#undef F_IMPL
#undef F_ARITY

#undef CHECK_EXPAND

    // ML99_assertIsTuple
    { ML99_EVAL(ML99_assertIsTuple(ML99_tuple(v(1, 2, 3)))); }
}
