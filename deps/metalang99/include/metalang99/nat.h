/**
 * @file
 * Natural numbers: [0; 255].
 *
 * Most of the time, natural numbers are used for iteration; they are not meant for CPU-bound tasks
 * such as Fibonacci numbers or factorials.
 */

#ifndef ML99_NAT_H
#define ML99_NAT_H

#include <metalang99/priv/bool.h>

#include <metalang99/nat/dec.h>
#include <metalang99/nat/div.h>
#include <metalang99/nat/eq.h>
#include <metalang99/nat/inc.h>

#include <metalang99/lang.h>

/**
 * \f$x + 1\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 6
 * ML99_inc(v(5))
 * @endcode
 *
 * @note If @p x is #ML99_NAT_MAX, the result is 0.
 */
#define ML99_inc(x) ML99_call(ML99_inc, x)

/**
 * \f$x - 1\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 4
 * ML99_dec(v(5))
 * @endcode
 *
 * @note If @p x is 0, the result is #ML99_NAT_MAX.
 */
#define ML99_dec(x) ML99_call(ML99_dec, x)

/**
 * Matches @p x against the two cases: if it is zero or positive.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * #define MATCH_Z_IMPL()  v(Billie)
 * #define MATCH_S_IMPL(x) v(Jean ~ x)
 *
 * // Billie
 * ML99_natMatch(v(0), v(MATCH_))
 *
 * // Jean ~ 122
 * ML99_natMatch(v(123), v(MATCH_))
 * @endcode
 *
 * @note This function calls @p f with #ML99_call, so no partial application occurs, and so
 * arity specifiers are not needed.
 */
#define ML99_natMatch(x, matcher) ML99_call(ML99_natMatch, x, matcher)

/**
 * The same as #ML99_natMatch but provides additional arguments to all branches.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * #define MATCH_Z_IMPL(x, y, z)    v(Billie ~ x y z)
 * #define MATCH_S_IMPL(n, x, y, z) v(Jean ~ n ~ x y z)
 *
 * // Billie ~ 1 2 3
 * ML99_natMatchWithArgs(v(0), v(MATCH_), v(1, 2, 3))
 *
 * // Jean ~ 122 ~ 1 2 3
 * ML99_natMatchWithArgs(v(123), v(MATCH_), v(1, 2, 3))
 * @endcode
 */
#define ML99_natMatchWithArgs(x, matcher, ...)                                                     \
    ML99_call(ML99_natMatchWithArgs, x, matcher, __VA_ARGS__)

/**
 * \f$x = y\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 1
 * ML99_natEq(v(5), v(5))
 *
 * // 0
 * ML99_natEq(v(3), v(8))
 * @endcode
 */
#define ML99_natEq(x, y) ML99_call(ML99_natEq, x, y)

/**
 * \f$x \neq y\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 0
 * ML99_natNeq(v(5), v(5))
 *
 * // 1
 * ML99_natNeq(v(3), v(8))
 * @endcode
 */
#define ML99_natNeq(x, y) ML99_call(ML99_natNeq, x, y)

/**
 * \f$x > y\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 1
 * ML99_greater(v(8), v(3))
 *
 * // 0
 * ML99_greater(v(3), v(8))
 * @endcode
 */
#define ML99_greater(x, y) ML99_call(ML99_greater, x, y)

/**
 * \f$x \geq y\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 1
 * ML99_greaterEq(v(8), v(8))
 *
 * // 0
 * ML99_greaterEq(v(3), v(8))
 * @endcode
 */
#define ML99_greaterEq(x, y) ML99_call(ML99_greaterEq, x, y)

/**
 * \f$x < y\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 1
 * ML99_lesser(v(3), v(8))
 *
 * // 0
 * ML99_lesser(v(8), v(3))
 * @endcode
 */
#define ML99_lesser(x, y) ML99_call(ML99_lesser, x, y)

/**
 * \f$x \leq y\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 1
 * ML99_lesserEq(v(8), v(8))
 *
 * // 0
 * ML99_lesserEq(v(8), v(3))
 * @endcode
 */
#define ML99_lesserEq(x, y) ML99_call(ML99_lesserEq, x, y)

/**
 * \f$x + y\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 11
 * ML99_add(v(5), v(6))
 * @endcode
 */
#define ML99_add(x, y) ML99_call(ML99_add, x, y)

/**
 * \f$x - y\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 6
 * ML99_sub(v(11), v(5))
 * @endcode
 */
#define ML99_sub(x, y) ML99_call(ML99_sub, x, y)

/**
 * \f$x * y\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 12
 * ML99_mul(v(3), v(4))
 * @endcode
 */
#define ML99_mul(x, y) ML99_call(ML99_mul, x, y)

/**
 * \f$\frac{x}{y}\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 3
 * ML99_div(v(12), v(4))
 * @endcode
 *
 * @note A compile-time error if \f$\frac{x}{y}\f$ is not a natural number.
 */
#define ML99_div(x, y) ML99_call(ML99_div, x, y)

/**
 * Like #ML99_div but returns `ML99_nothing()` is @p x is not divisible by @p y,
 * otherwise `ML99_just(result)`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // ML99_just(3)
 * ML99_divChecked(v(12), v(4))
 *
 * // ML99_nothing()
 * ML99_divChecked(v(14), v(5))
 *
 * // ML99_nothing()
 * ML99_divChecked(v(1), v(0))
 * @endcode
 */
#define ML99_divChecked(x, y) ML99_call(ML99_divChecked, x, y)

/**
 * Computes the remainder of division.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 2
 * ML99_mod(v(8), v(3))
 * @endcode
 *
 * @note A compile-time error if @p y is 0.
 */
#define ML99_mod(x, y) ML99_call(ML99_mod, x, y)

/**
 * \f$x + y + z\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 15
 * ML99_add3(v(1), v(6), v(8))
 * @endcode
 */
#define ML99_add3(x, y, z) ML99_call(ML99_add3, x, y, z)

/**
 * \f$x - y - z\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 3
 * ML99_sub3(v(8), v(2), v(3))
 * @endcode
 */
#define ML99_sub3(x, y, z) ML99_call(ML99_sub3, x, y, z)

/**
 * \f$x * y * z\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 24
 * ML99_mul3(v(2), v(3), v(4))
 * @endcode
 */
#define ML99_mul3(x, y, z) ML99_call(ML99_mul3, x, y, z)

/**
 * \f$\frac{(\frac{x}{y})}{z}\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 5
 * ML99_div(v(30), v(3), v(2))
 * @endcode
 *
 * @note A compile-time error if \f$\frac{(\frac{x}{y})}{z}\f$ is not a natural number.
 */
#define ML99_div3(x, y, z) ML99_call(ML99_div3, x, y, z)

/**
 * \f$min(x, y)\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 5
 * ML99_min(v(5), v(7))
 * @endcode
 */
#define ML99_min(x, y) ML99_call(ML99_min, x, y)

/**
 * \f$max(x, y)\f$
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * // 7
 * ML99_max(v(5), v(7))
 * @endcode
 */
#define ML99_max(x, y) ML99_call(ML99_max, x, y)

/**
 * Emits a fatal error if @p x is not a natural number, otherwise results in emptiness.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/nat.h>
 *
 * #define F_IMPL(x) ML99_TERMS(ML99_assertIsNat(v(x)), ML99_inc(v(x)))
 *
 * // 6
 * ML99_call(F, v(5))
 *
 * // A compile-time number mismatch error.
 * ML99_call(F, v(blah))
 * @endcode
 */
#define ML99_assertIsNat(x) ML99_call(ML99_assertIsNat, x)

#define ML99_INC(x)            ML99_PRIV_INC(x)
#define ML99_DEC(x)            ML99_PRIV_DEC(x)
#define ML99_NAT_EQ(x, y)      ML99_PRIV_NAT_EQ(x, y)
#define ML99_NAT_NEQ(x, y)     ML99_PRIV_NOT(ML99_NAT_EQ(x, y))
#define ML99_DIV_CHECKED(x, y) ML99_PRIV_DIV_CHECKED(x, y)

/**
 * The maximum value of a natural number, currently 255.
 */
#define ML99_NAT_MAX 255

#ifndef DOXYGEN_IGNORE

// Pattern matching {

#define ML99_natMatch_IMPL(x, matcher)                                                             \
    ML99_PRIV_IF(                                                                                  \
        ML99_NAT_EQ(x, 0),                                                                         \
        ML99_callUneval(matcher##Z, ),                                                             \
        ML99_callUneval(matcher##S, ML99_DEC(x)))

#define ML99_natMatchWithArgs_IMPL(x, matcher, ...)                                                \
    ML99_PRIV_IF(                                                                                  \
        ML99_NAT_EQ(x, 0),                                                                         \
        ML99_callUneval(matcher##Z, __VA_ARGS__),                                                  \
        ML99_callUneval(matcher##S, ML99_DEC(x), __VA_ARGS__))
// } (Pattern matching)

// Comparison operators {

#define ML99_natEq_IMPL(x, y)  v(ML99_NAT_EQ(x, y))
#define ML99_natNeq_IMPL(x, y) v(ML99_NAT_NEQ(x, y))

#define ML99_lesser_IMPL(x, y)                                                                     \
    ML99_PRIV_IF(                                                                                  \
        ML99_NAT_EQ(y, 0),                                                                         \
        v(ML99_PRIV_FALSE()),                                                                      \
        ML99_PRIV_IF(                                                                              \
            ML99_NAT_EQ(x, ML99_DEC(y)),                                                           \
            v(ML99_PRIV_TRUE()),                                                                   \
            ML99_callUneval(ML99_lesser, x, ML99_DEC(y))))

#define ML99_lesserEq_IMPL(x, y) ML99_greaterEq_IMPL(y, x)

#define ML99_greater_IMPL(x, y) ML99_lesser_IMPL(y, x)
#define ML99_greaterEq_IMPL(x, y)                                                                  \
    ML99_PRIV_IF(ML99_NAT_EQ(x, y), v(ML99_PRIV_TRUE()), ML99_greater_IMPL(x, y))
// } (Comparison operators)

// Arithmetical operators {

#define ML99_inc_IMPL(x) v(ML99_INC(x))
#define ML99_dec_IMPL(x) v(ML99_DEC(x))

#define ML99_add_IMPL(x, y)                                                                        \
    ML99_PRIV_IF(ML99_NAT_EQ(y, 0), v(x), ML99_callUneval(ML99_add, ML99_INC(x), ML99_DEC(y)))
#define ML99_sub_IMPL(x, y)                                                                        \
    ML99_PRIV_IF(ML99_NAT_EQ(y, 0), v(x), ML99_callUneval(ML99_sub, ML99_DEC(x), ML99_DEC(y)))
#define ML99_mul_IMPL(x, y)                                                                        \
    ML99_PRIV_IF(ML99_NAT_EQ(y, 0), v(0), ML99_add(v(x), ML99_callUneval(ML99_mul, x, ML99_DEC(y))))

#define ML99_add3_IMPL(x, y, z) ML99_add(ML99_add_IMPL(x, y), v(z))
#define ML99_sub3_IMPL(x, y, z) ML99_sub(ML99_sub_IMPL(x, y), v(z))
#define ML99_mul3_IMPL(x, y, z) ML99_mul(ML99_mul_IMPL(x, y), v(z))
#define ML99_div3_IMPL(x, y, z) ML99_div(ML99_div_IMPL(x, y), v(z))

#define ML99_min_IMPL(x, y) ML99_call(ML99_if, ML99_lesser_IMPL(x, y), v(x, y))
#define ML99_max_IMPL(x, y) ML99_call(ML99_if, ML99_lesser_IMPL(x, y), v(y, x))

#define ML99_divChecked_IMPL(x, y) v(ML99_DIV_CHECKED(x, y))

// ML99_mod_IMPL {

#define ML99_mod_IMPL(x, y)                                                                        \
    ML99_PRIV_IF(                                                                                  \
        ML99_NAT_EQ(y, 0),                                                                         \
        ML99_fatal(ML99_mod, modulo by 0),                                                         \
        ML99_PRIV_modAux_IMPL(x, y, 0))

#define ML99_PRIV_modAux_IMPL(x, y, acc)                                                           \
    ML99_PRIV_IF(                                                                                  \
        ML99_PRIV_OR(ML99_NAT_EQ(x, 0), ML99_IS_JUST(ML99_DIV_CHECKED(x, y))),                     \
        v(acc),                                                                                    \
        ML99_callUneval(ML99_PRIV_modAux, ML99_DEC(x), y, ML99_INC(acc)))
// } (ML99_mod_IMPL)

// } (Arithmetical operators)

#define ML99_assertIsNat_IMPL(x)                                                                   \
    ML99_PRIV_IF(                                                                                  \
        ML99_PRIV_NAT_EQ(x, x),                                                                    \
        v(ML99_PRIV_EMPTY()),                                                                      \
        ML99_PRIV_ASSERT_IS_NAT_FATAL(x, ML99_NAT_MAX))

// clang-format off
#define ML99_PRIV_ASSERT_IS_NAT_FATAL(x, max) ML99_fatal(ML99_assertIsNat, x must be within [0; max])
// clang-format on

// Arity specifiers {

#define ML99_inc_ARITY              1
#define ML99_dec_ARITY              1
#define ML99_natMatch_ARITY         2
#define ML99_natMatchWithArgs_ARITY 3
#define ML99_natEq_ARITY            2
#define ML99_natNeq_ARITY           2
#define ML99_greater_ARITY          2
#define ML99_greaterEq_ARITY        2
#define ML99_lesser_ARITY           2
#define ML99_lesserEq_ARITY         2
#define ML99_add_ARITY              2
#define ML99_sub_ARITY              2
#define ML99_mul_ARITY              2
#define ML99_div_ARITY              2
#define ML99_divChecked_ARITY       2
#define ML99_mod_ARITY              2
#define ML99_add3_ARITY             3
#define ML99_sub3_ARITY             3
#define ML99_mul3_ARITY             3
#define ML99_div3_ARITY             3
#define ML99_min_ARITY              2
#define ML99_max_ARITY              2
#define ML99_assertIsNat_ARITY      1
// } (Arity specifiers)

#endif // DOXYGEN_IGNORE

#endif // ML99_NAT_H
