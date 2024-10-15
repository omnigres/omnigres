// `...` is sometimes used to workaround a TCC bug, see
// <https://github.com/Hirrolot/datatype99/issues/10#issuecomment-830813172>.

#include <metalang99/assert.h>
#include <metalang99/nat.h>

int main(void) {

#define MATCH_Z_IMPL(...) v(88) // `...` due to the TCC's bug.
#define MATCH_S_IMPL(x)   v(x)

    // ML99_natMatch
    {
        ML99_ASSERT_EQ(ML99_natMatch(v(0), v(MATCH_)), v(88));
        ML99_ASSERT_EQ(ML99_natMatch(v(123), v(MATCH_)), v(122));
    }

#undef MATCH_Z_IMPL
#undef MATCH_S_IMPL

#define MATCH_Z_IMPL(x, y, z)    v(ML99_ASSERT_UNEVAL(x == 1 && y == 2 && z == 3))
#define MATCH_S_IMPL(n, x, y, z) v(ML99_ASSERT_UNEVAL(n == 122 && x == 1 && y == 2 && z == 3))

    // ML99_natMatchWithArgs
    {
        ML99_EVAL(ML99_natMatchWithArgs(v(0), v(MATCH_), v(1, 2, 3)));
        ML99_EVAL(ML99_natMatchWithArgs(v(123), v(MATCH_), v(1, 2, 3)));
    }

#undef MATCH_Z_IMPL
#undef MATCH_S_IMPL

    // ML99_inc
    {
        ML99_ASSERT_EQ(ML99_inc(v(0)), v(1));
        ML99_ASSERT_EQ(ML99_inc(v(15)), v(16));
        ML99_ASSERT_EQ(ML99_inc(v(198)), v(199));
        ML99_ASSERT_EQ(ML99_inc(v(254)), v(ML99_NAT_MAX));
        ML99_ASSERT_EQ(ML99_inc(v(ML99_NAT_MAX)), v(0));
    }

    // ML99_INC
    {
        ML99_ASSERT_UNEVAL(ML99_INC(0) == 1);
        ML99_ASSERT_UNEVAL(ML99_INC(15) == 16);
        ML99_ASSERT_UNEVAL(ML99_INC(254) == ML99_NAT_MAX);
    }

    // ML99_dec
    {
        ML99_ASSERT_EQ(ML99_dec(v(0)), v(ML99_NAT_MAX));
        ML99_ASSERT_EQ(ML99_dec(v(1)), v(0));
        ML99_ASSERT_EQ(ML99_dec(v(71)), v(70));
        ML99_ASSERT_EQ(ML99_dec(v(201)), v(200));
        ML99_ASSERT_EQ(ML99_dec(v(ML99_NAT_MAX)), v(254));
    }

    // ML99_DEC
    {
        ML99_ASSERT_UNEVAL(ML99_DEC(0) == ML99_NAT_MAX);
        ML99_ASSERT_UNEVAL(ML99_DEC(1) == 0);
        ML99_ASSERT_UNEVAL(ML99_DEC(ML99_NAT_MAX) == 254);
    }

    // ML99_natEq
    {
        ML99_ASSERT(ML99_natEq(v(0), v(0)));
        ML99_ASSERT(ML99_natEq(v(18), v(18)));
        ML99_ASSERT(ML99_natEq(v(183), v(183)));
        ML99_ASSERT(ML99_natEq(v(ML99_NAT_MAX), v(ML99_NAT_MAX)));

        ML99_ASSERT(ML99_not(ML99_natEq(v(0), v(1))));
        ML99_ASSERT(ML99_not(ML99_natEq(v(198), v(91))));
    }

    // ML99_NAT_EQ
    {
        ML99_ASSERT_UNEVAL(ML99_NAT_EQ(18, 18));
        ML99_ASSERT_UNEVAL(!ML99_NAT_EQ(198, 91));
    }

    // ML99_natNeq
    {
        ML99_ASSERT(ML99_natNeq(v(0), v(1)));
        ML99_ASSERT(ML99_natNeq(v(0), v(168)));
        ML99_ASSERT(ML99_natNeq(v(1), v(34)));
        ML99_ASSERT(ML99_natNeq(v(184), v(381)));
        ML99_ASSERT(ML99_natNeq(v(3), v(101)));

        ML99_ASSERT(ML99_not(ML99_natNeq(v(0), v(0))));
        ML99_ASSERT(ML99_not(ML99_natNeq(v(101), v(101))));
    }

    // ML99_NAT_NEQ
    {
        ML99_ASSERT_UNEVAL(ML99_NAT_NEQ(0, 168));
        ML99_ASSERT_UNEVAL(!ML99_NAT_NEQ(101, 101));
    }

    // ML99_greater
    {
        ML99_ASSERT(ML99_greater(v(1), v(0)));
        ML99_ASSERT(ML99_greater(v(ML99_NAT_MAX), v(0)));
        ML99_ASSERT(ML99_greater(v(5), v(4)));
        ML99_ASSERT(ML99_greater(v(147), v(80)));
        ML99_ASSERT(ML99_greater(v(217), v(209)));

        ML99_ASSERT(ML99_not(ML99_greater(v(0), v(13))));
        ML99_ASSERT(ML99_not(ML99_greater(v(17), v(120))));
    }

    // ML99_lesser
    {
        ML99_ASSERT(ML99_lesser(v(0), v(1)));
        ML99_ASSERT(ML99_lesser(v(0), v(ML99_NAT_MAX)));
        ML99_ASSERT(ML99_lesser(v(19), v(25)));
        ML99_ASSERT(ML99_lesser(v(109), v(110)));
        ML99_ASSERT(ML99_lesser(v(10), v(208)));

        ML99_ASSERT(ML99_not(ML99_lesser(v(12), v(0))));
        ML99_ASSERT(ML99_not(ML99_lesser(v(123), v(123))));
    }

    // ML99_greaterEq
    {
        ML99_ASSERT(ML99_greaterEq(v(0), v(0)));
        ML99_ASSERT(ML99_greaterEq(v(18), v(18)));
        ML99_ASSERT(ML99_greaterEq(v(175), v(175)));
        ML99_ASSERT(ML99_greaterEq(v(ML99_NAT_MAX), v(ML99_NAT_MAX)));

        ML99_ASSERT(ML99_greaterEq(v(1), v(0)));
        ML99_ASSERT(ML99_greaterEq(v(ML99_NAT_MAX), v(0)));
        ML99_ASSERT(ML99_greaterEq(v(19), v(10)));
        ML99_ASSERT(ML99_greaterEq(v(178), v(177)));

        ML99_ASSERT(ML99_not(ML99_greaterEq(v(0), v(7))));
        ML99_ASSERT(ML99_not(ML99_greaterEq(v(1), v(19))));
    }

    // ML99_lesserEq
    {
        ML99_ASSERT(ML99_lesserEq(v(0), v(0)));
        ML99_ASSERT(ML99_lesserEq(v(2), v(2)));
        ML99_ASSERT(ML99_lesserEq(v(1), v(1)));
        ML99_ASSERT(ML99_lesserEq(v(25), v(25)));
        ML99_ASSERT(ML99_lesserEq(v(198), v(198)));

        ML99_ASSERT(ML99_lesserEq(v(0), v(1)));
        ML99_ASSERT(ML99_lesserEq(v(0), v(ML99_NAT_MAX)));
        ML99_ASSERT(ML99_lesserEq(v(18), v(27)));
        ML99_ASSERT(ML99_lesserEq(v(82), v(90)));
        ML99_ASSERT(ML99_lesserEq(v(145), v(146)));
        ML99_ASSERT(ML99_lesserEq(v(181), v(ML99_NAT_MAX)));

        ML99_ASSERT(ML99_not(ML99_lesserEq(v(7), v(0))));
        ML99_ASSERT(ML99_not(ML99_lesserEq(v(182), v(181))));
    }

    // ML99_add
    {
        ML99_ASSERT_EQ(ML99_add(v(0), v(0)), v(0));
        ML99_ASSERT_EQ(ML99_add(v(19), v(83)), v(19 + 83));
        ML99_ASSERT_EQ(ML99_add(v(8), v(4)), v(8 + 4));
        ML99_ASSERT_EQ(ML99_add(v(1), v(254)), v(1 + 254));
    }

    // ML99_sub
    {
        ML99_ASSERT_EQ(ML99_sub(v(1), v(1)), v(1 - 1));
        ML99_ASSERT_EQ(ML99_sub(v(5), v(3)), v(5 - 3));
        ML99_ASSERT_EQ(ML99_sub(v(105), v(19)), v(105 - 19));
        ML99_ASSERT_EQ(ML99_sub(v(ML99_NAT_MAX), v(40)), v(ML99_NAT_MAX - 40));
    }

    // ML99_mul
    {
        ML99_ASSERT_EQ(ML99_mul(v(11), v(0)), v(0));
        ML99_ASSERT_EQ(ML99_mul(v(0), v(11)), v(0));
        ML99_ASSERT_EQ(ML99_mul(v(15), v(8)), v(15 * 8));
        ML99_ASSERT_EQ(ML99_mul(v(ML99_NAT_MAX), v(1)), v(ML99_NAT_MAX * 1));
    }

    // ML99_div
    {
        ML99_ASSERT_EQ(ML99_div(v(15), v(1)), v(15));
        ML99_ASSERT_EQ(ML99_div(v(15), v(15)), v(1));
        ML99_ASSERT_EQ(ML99_div(v(45), v(3)), v(45 / 3));
        ML99_ASSERT_EQ(ML99_div(v(ML99_NAT_MAX), v(5)), v(ML99_NAT_MAX / 5));
    }

    // ML99_divChecked
    {
        ML99_ASSERT(ML99_maybeEq(v(ML99_natEq), ML99_divChecked(v(15), v(1)), ML99_just(v(15))));
        ML99_ASSERT(ML99_maybeEq(v(ML99_natEq), ML99_divChecked(v(15), v(15)), ML99_just(v(1))));
        ML99_ASSERT(ML99_maybeEq(v(ML99_natEq), ML99_divChecked(v(45), v(3)), ML99_just(v(15))));
        ML99_ASSERT(
            ML99_maybeEq(v(ML99_natEq), ML99_divChecked(v(ML99_NAT_MAX), v(5)), ML99_just(v(51))));

        ML99_ASSERT(ML99_maybeEq(v(ML99_natEq), ML99_divChecked(v(4), v(0)), ML99_nothing()));
        ML99_ASSERT(ML99_maybeEq(v(ML99_natEq), ML99_divChecked(v(3), v(27)), ML99_nothing()));
        ML99_ASSERT(ML99_maybeEq(v(ML99_natEq), ML99_divChecked(v(166), v(9)), ML99_nothing()));
        ML99_ASSERT(ML99_maybeEq(v(ML99_natEq), ML99_divChecked(v(0), v(11)), ML99_nothing()));
    }

    // ML99_DIV_CHECKED
    {
        ML99_ASSERT(ML99_maybeEq(v(ML99_natEq), v(ML99_DIV_CHECKED(15, 1)), ML99_just(v(15))));
        ML99_ASSERT(ML99_maybeEq(v(ML99_natEq), v(ML99_DIV_CHECKED(4, 0)), ML99_nothing()));
    }

    // ML99_mod
    {
        ML99_ASSERT_EQ(ML99_mod(v(0), v(1)), v(0 % 1));
        ML99_ASSERT_EQ(ML99_mod(v(0), v(123)), v(0 % 123));

        ML99_ASSERT_EQ(ML99_mod(v(1), v(28)), v(1 % 28));
        ML99_ASSERT_EQ(ML99_mod(v(1), v(123)), v(1 % 123));

        ML99_ASSERT_EQ(ML99_mod(v(1), v(1)), v(0));
        ML99_ASSERT_EQ(ML99_mod(v(16), v(4)), v(0));
        ML99_ASSERT_EQ(ML99_mod(v(ML99_NAT_MAX), v(ML99_NAT_MAX)), v(0));

        ML99_ASSERT_EQ(ML99_mod(v(8), v(3)), v(8 % 3));
        ML99_ASSERT_EQ(ML99_mod(v(10), v(4)), v(10 % 4));
        ML99_ASSERT_EQ(ML99_mod(v(101), v(7)), v(101 % 7));

        ML99_ASSERT_EQ(ML99_mod(v(13), v(14)), v(13 % 14));
        ML99_ASSERT_EQ(ML99_mod(v(20), v(36)), v(20 % 36));
        ML99_ASSERT_EQ(ML99_mod(v(16), v(ML99_NAT_MAX)), v(16 % ML99_NAT_MAX));
    }

    // ML99_add3, ML99_sub3, ML99_mul3, ML99_div3
    {
        ML99_ASSERT_EQ(ML99_add3(v(8), v(2), v(4)), v(8 + 2 + 4));
        ML99_ASSERT_EQ(ML99_sub3(v(14), v(1), v(7)), v(14 - 1 - 7));
        ML99_ASSERT_EQ(ML99_mul3(v(3), v(2), v(6)), v(3 * 2 * 6));
        ML99_ASSERT_EQ(ML99_div3(v(30), v(2), v(3)), v(30 / 2 / 3));
    }

    // ML99_min
    {
        ML99_ASSERT_EQ(ML99_min(v(0), v(1)), v(0));
        ML99_ASSERT_EQ(ML99_min(v(5), v(7)), v(5));
        ML99_ASSERT_EQ(ML99_min(v(200), v(ML99_NAT_MAX)), v(200));
    }

    // ML99_max
    {
        ML99_ASSERT_EQ(ML99_max(v(0), v(1)), v(1));
        ML99_ASSERT_EQ(ML99_max(v(5), v(7)), v(7));
        ML99_ASSERT_EQ(ML99_max(v(200), v(ML99_NAT_MAX)), v(ML99_NAT_MAX));
    }

    // ML99_assertIsNat
    {
        ML99_EVAL(ML99_assertIsNat(v(0)))
        ML99_EVAL(ML99_assertIsNat(v(13)))
        ML99_EVAL(ML99_assertIsNat(v(255)))
    }
}
