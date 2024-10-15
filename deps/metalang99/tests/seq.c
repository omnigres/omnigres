#include <metalang99/assert.h>
#include <metalang99/seq.h>

int main(void) {

    // ML99_seqGet
    {
        ML99_ASSERT_EQ(ML99_seqGet(0)(v((19))), v(19));
        ML99_ASSERT_EQ(ML99_seqGet(0)(v((19)(8))), v(19));
        ML99_ASSERT_EQ(ML99_seqGet(0)(v((19)(8)(7378))), v(19));

        ML99_ASSERT_EQ(ML99_seqGet(1)(v((19)(8))), v(8));
        ML99_ASSERT_EQ(ML99_seqGet(2)(v((19)(8)(7378))), v(7378));
        ML99_ASSERT_EQ(ML99_seqGet(3)(v((19)(8)(7378)(10))), v(10));
        ML99_ASSERT_EQ(ML99_seqGet(4)(v((19)(8)(7378)(10)(121))), v(121));
        ML99_ASSERT_EQ(ML99_seqGet(5)(v((19)(8)(7378)(10)(121)(1))), v(1));
        ML99_ASSERT_EQ(ML99_seqGet(6)(v((19)(8)(7378)(10)(121)(1)(80))), v(80));
        ML99_ASSERT_EQ(ML99_seqGet(7)(v((19)(8)(7378)(10)(121)(1)(80)(23))), v(23));
    }

    // ML99_SEQ_GET
    {
        ML99_ASSERT_UNEVAL(ML99_SEQ_GET(0)((19)) == 19);
        ML99_ASSERT_UNEVAL(ML99_SEQ_GET(0)((19)(8)) == 19);
        ML99_ASSERT_UNEVAL(ML99_SEQ_GET(0)((19)(8)(7378)) == 19);

        ML99_ASSERT_UNEVAL(ML99_SEQ_GET(1)((19)(8)) == 8);
        ML99_ASSERT_UNEVAL(ML99_SEQ_GET(1)((19)(8)(7378)) == 8);
    }

#define CHECK_TAIL(a)       a == 51 && CHECK_TAIL_AUX_0
#define CHECK_TAIL_AUX_0(b) b == 3 && CHECK_TAIL_AUX_1
#define CHECK_TAIL_AUX_1(c) c == 9

    // ML99_seqTail
    {
        ML99_ASSERT_UNEVAL(CHECK_TAIL ML99_EVAL(ML99_seqTail(v((9191)(51)(3)(9)))));
        ML99_ASSERT_EMPTY(ML99_seqTail(v((~, ~, ~))));
    }

    // ML99_SEQ_TAIL
    {
        ML99_ASSERT_UNEVAL(CHECK_TAIL ML99_SEQ_TAIL((9191)(51)(3)(9)));
        ML99_ASSERT_EMPTY_UNEVAL(ML99_SEQ_TAIL((~, ~, ~)));
    }

#undef CHECK_TAIL
#undef CHECK_TAIL_AUX_0
#undef CHECK_TAIL_AUX_1

    // ML99_seqIsEmpty
    {
        ML99_ASSERT(ML99_seqIsEmpty(v()));

        ML99_ASSERT(ML99_not(ML99_seqIsEmpty(v((~, ~, ~)))));
        ML99_ASSERT(ML99_not(ML99_seqIsEmpty(v((~)(~)))));
        ML99_ASSERT(ML99_not(ML99_seqIsEmpty(v((~)(~)(~)))));
    }

    // ML99_SEQ_IS_EMPTY
    {
        ML99_ASSERT_UNEVAL(ML99_SEQ_IS_EMPTY());

        ML99_ASSERT_UNEVAL(ML99_NOT(ML99_SEQ_IS_EMPTY((~, ~, ~))));
        ML99_ASSERT_UNEVAL(ML99_NOT(ML99_SEQ_IS_EMPTY((~)(~))));
        ML99_ASSERT_UNEVAL(ML99_NOT(ML99_SEQ_IS_EMPTY((~)(~)(~))));
    }

#define CHECK_EXPAND(...) CHECK(__VA_ARGS__)

#define CHECK(_, x, y, z) ML99_ASSERT_UNEVAL(x == 1987 && y == 2987 && z == 3987)
#define F_IMPL(x)         v(, x##987)
#define F_ARITY           1

    // ML99_seqForEach
    {
        ML99_ASSERT_EMPTY(ML99_seqForEach(v(F), v()));
        CHECK_EXPAND(ML99_EVAL(ML99_seqForEach(v(F), v((1)(2)(3)))));
    }

#undef CHECK
#undef F_IMPL
#undef F_ARITY

#define CHECK(_, x, y, z) ML99_ASSERT_UNEVAL(x == 10 && y == 21 && z == 32)
#define F_IMPL(i, x)      v(, ), v(x##i)
#define F_ARITY           2

    // ML99_seqForEachI
    {
        ML99_ASSERT_EMPTY(ML99_seqForEachI(v(F), v()));
        CHECK_EXPAND(ML99_EVAL(ML99_seqForEachI(v(F), v((1)(2)(3)))));
    }

#undef CHECK
#undef F_IMPL
#undef F_ARITY

#undef CHECK_EXPAND
}
