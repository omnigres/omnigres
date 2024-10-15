// `...` is sometimes used to workaround a TCC bug, see
// <https://github.com/Hirrolot/datatype99/issues/10#issuecomment-830813172>.

#include <metalang99/assert.h>
#include <metalang99/util.h>

int main(void) {

    ML99_ASSERT_EMPTY_UNEVAL(ML99_EVAL(v()));

#ifndef __TINYC__
#define F_IMPL() v(123)
#else
#define F_IMPL(...) v(123) // `...` due to the TCC's bug.
#endif

    // A function with zero arguments
    {
        ML99_ASSERT_EQ(ML99_call(F, v()), v(123));
        ML99_ASSERT_EQ(ML99_call(v(F), v()), v(123));
        ML99_ASSERT_EQ(ML99_callUneval(F, ), v(123));
    }

#undef F_IMPL

#define F_IMPL(x, y, z) v(x##y##z)
#define BAR_IMPL(x, y)  v(x + y)

    // Regular usage of the metalanguage
    {
        ML99_ASSERT_EQ(ML99_call(BAR, v(5), v(7)), v(5 + 7));
        ML99_ASSERT_EQ(ML99_call(ML99_call(F, v(B), v(A), v(R)), v(6), v(11)), v(6 + 11));

        ML99_ASSERT_EQ(ML99_call(BAR, v(5, 7)), v(5 + 7));
        ML99_ASSERT_EQ(ML99_call(ML99_call(F, v(B, A, R)), v(6, 11)), v(6 + 11));

        ML99_ASSERT_EQ(ML99_callUneval(BAR, 5, 7), v(5 + 7));
    }

#undef F_IMPL
#undef BAR_IMPL

// Even if a term in the argument position evaluates to more than one terms, they should be appended
// to each other but not interspersed with a comma.
#define F_IMPL(...) ML99_TERMS(v(1), v(2), v(3)) // `...` due to the TCC's bug.
#define BAR_IMPL(x) v()

    ML99_EVAL(ML99_call(BAR, ML99_call(F, v())))

#undef F_IMPL
#undef BAR_IMPL

// Recursion might arise from a higher-order macro, if `op` invokes `F`, but nonetheless the
// second call to `F` must be performed as expected.
#define F_IMPL(op) ML99_call(op, v(123))
#define OP_IMPL(x) ML99_call(F, v(ID))
#define ID_IMPL(x) v(x)

    ML99_ASSERT_EQ(ML99_call(F, v(OP)), v(123));

#undef F_IMPL
#undef OP_IMPL
#undef ID_IMPL

    // ML99_abort
    {
        ML99_ASSERT_EMPTY(ML99_abort(v()));
        ML99_ASSERT_EQ(ML99_abort(v(815057)), v(815057));
        ML99_ASSERT_UNEVAL(ML99_EVAL(v(~), ML99_abort(v(123)), v(~)) == 123);

// Ensure that `ML99_abort` also works correctly after some evaluations.
#define F_IMPL(...) ML99_call(G, v(1, 2), ML99_call(H, v(123))) // `...` due to the TCC's bug.
#define G_IMPL(_1, _2, _123_plus_1)                                                                \
    ML99_abort(v(ML99_ASSERT_UNEVAL(_1 == 1 && _2 == 2 && _123_plus_1 == 123 + 1)))
#define H_IMPL(a) v(a + 1)

        ML99_EVAL(ML99_call(F, v()));

#undef F_IMPL
#undef G_IMPL
#undef H_IMPL

        // Ensure that `ML99_abort` immediately aborts interpretation even in an argument position.
        ML99_ASSERT_EQ(ML99_call(NonExistingF, ML99_abort(v(123))), v(123));
    }

    // Partial application
    {

#ifndef __TINYC__
#define F_IMPL() v(123)
#else
#define F_IMPL(...) v(123) // // `...` due to the TCC's bug.
#endif

#define F_ARITY 1

        // The arity of a function with zero arguments must be 1
        { ML99_ASSERT_EQ(ML99_appl(v(F), v()), v(123)); }

#undef F_IMPL
#undef F_ARITY

#define F_IMPL(a, b, c, d) v(a##b##c##d)
#define F_ARITY            4

        // Regular usage of partial application
        {
            ML99_ASSERT_EQ(
                ML99_appl(ML99_appl(ML99_appl(ML99_appl(v(F), v(10)), v(5)), v(7)), v(8)),
                v(10578));
            ML99_ASSERT_EQ(
                ML99_appl(ML99_appl(ML99_appl2(v(F), v(10), v(5)), v(7)), v(8)),
                v(10578));
            ML99_ASSERT_EQ(ML99_appl(ML99_appl3(v(F), v(10), v(5), v(7)), v(8)), v(10578));
            ML99_ASSERT_EQ(ML99_appl4(v(F), v(10), v(5), v(7), v(8)), v(10578));
        }

#undef F_IMPL
#undef F_ARITY

#define F_ARITY 255

        // The maximum arity
        {
            ML99_EVAL(ML99_empty(ML99_appl(v(F), v(~))))
            ML99_EVAL(ML99_empty(ML99_appl2(v(F), v(~), v(~))))
            ML99_EVAL(ML99_empty(ML99_appl3(v(F), v(~), v(~), v(~))))
        }

#undef F_ARITY
    }

#define F_IMPL(x) v((x + 1))
#define G_IMPL(x) v((x * 8))

#define F_ARITY 1
#define G_ARITY 1

    // ML99_compose
    { ML99_ASSERT_EQ(ML99_appl(ML99_compose(v(F), v(G)), v(3)), v((3 * 8) + 1)); }

#undef F_IMPL
#undef G_IMPL

#undef F_ARITY
#undef G_ARITY

#define F_IMPL(x) v(x)

#define PROG ML99_TERMS(v(1), v(, ), v(2), v(, ), ML99_call(F, v(7)))

#define CHECK(...)         CHECK_AUX(__VA_ARGS__)
#define CHECK_AUX(a, b, c) ML99_ASSERT_UNEVAL(a == 1 && b == 2 && c == 7)

    // ML99_QUOTE
    { CHECK(ML99_EVAL(ML99_EVAL(ML99_QUOTE(PROG)))); }

#undef F_IMPL

#undef PROG

#undef CHECK
#undef CHECK_AUX
}
