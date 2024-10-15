/**
 * @file
 * Variadic arguments: `x, y, z`.
 *
 * @note Variadics are more time and space-efficient than lists, but export less functionality; if a
 * needed function is missed, invoking #ML99_list and then manipulating with the resulting Cons-list
 * might be helpful.
 */

#ifndef ML99_VARIADICS_H
#define ML99_VARIADICS_H

#include <metalang99/nat/inc.h>
#include <metalang99/priv/util.h>

#include <metalang99/lang.h>

/**
 * Computes a count of its arguments.
 *
 * At most 63 arguments are acceptable.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/variadics.h>
 *
 * // 3
 * ML99_variadicsCount(v(~, ~, ~))
 *
 * // 1
 * ML99_variadicsCount()
 * @endcode
 */
#define ML99_variadicsCount(...) ML99_call(ML99_variadicsCount, __VA_ARGS__)

/**
 * Tells if it received only one argument or not.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/variadics.h>
 *
 * // 1
 * ML99_variadicsIsSingle(v(~))
 *
 * // 0
 * ML99_variadicsIsSingle(v(~, ~, ~))
 * @endcode
 */
#define ML99_variadicsIsSingle(...) ML99_call(ML99_variadicsIsSingle, __VA_ARGS__)

/**
 * Expands to a metafunction extracting the @p i -indexed argument.
 *
 * @p i can range from 0 to 7, inclusively.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/variadics.h>
 *
 * // 2
 * ML99_variadicsGet(1)(v(1, 2, 3))
 * @endcode
 */
#define ML99_variadicsGet(i) ML99_PRIV_CAT(ML99_PRIV_variadicsGet_, i)

/**
 * Extracts the tail of its arguments.
 *
 * At least two arguments must be specified.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/variadics.h>
 *
 * // 2, 3
 * ML99_variadicsTail(v(1, 2, 3))
 * @endcode
 */
#define ML99_variadicsTail(...) ML99_call(ML99_variadicsTail, __VA_ARGS__)

/**
 * Applies @p f to each argument.
 *
 * The result is `ML99_appl(f, x1) ... ML99_appl(f, xN)`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/variadics.h>
 *
 * #define F_IMPL(x) v(@x)
 * #define F_ARITY   1
 *
 * // @x @y @z
 * ML99_variadicsForEach(v(F), v(x, y, z))
 * @endcode
 */
#define ML99_variadicsForEach(f, ...) ML99_call(ML99_variadicsForEach, f, __VA_ARGS__)

/**
 * Applies @p f to each argument with an index.
 *
 * The result is `ML99_appl2(f, x1, 0) ... ML99_appl2(f, xN, N - 1)`.
 *
 * @code
 * #include <metalang99/variadics.h>
 *
 * #define F_IMPL(x, i) v(@x##i)
 * #define F_ARITY      2
 *
 * // @x0 @y1 @z2
 * ML99_variadicsForEachI(v(F), v(x, y, z))
 * @endcode
 */
#define ML99_variadicsForEachI(f, ...) ML99_call(ML99_variadicsForEachI, f, __VA_ARGS__)

/**
 * Overloads @p f on a number of arguments.
 *
 * This function counts the number of provided arguments, appends it to @p f and calls the resulting
 * macro identifier with provided arguments.
 *
 * At most 63 variadic arguments are acceptable.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/variadics.h>
 *
 * #define X(...)    ML99_OVERLOAD(X_, __VA_ARGS__)
 * #define X_1(a)    Billie & a
 * #define X_2(a, b) Jean & a & b
 *
 * // Billie & 4
 * X(4)
 *
 * // Jean & 5 & 6
 * X(5, 6)
 * @endcode
 *
 * @note @p f need not be postfixed with `_IMPL`. It is literally invoked as `ML99_CAT(f,
 * ML99_VARIADICS_COUNT(...))(...)`.
 */
#define ML99_OVERLOAD(f, ...) ML99_PRIV_CAT(f, ML99_PRIV_VARIADICS_COUNT(__VA_ARGS__))(__VA_ARGS__)

#define ML99_VARIADICS_COUNT(...)     ML99_PRIV_VARIADICS_COUNT(__VA_ARGS__)
#define ML99_VARIADICS_IS_SINGLE(...) ML99_PRIV_NOT(ML99_PRIV_CONTAINS_COMMA(__VA_ARGS__))
#define ML99_VARIADICS_GET(i)         ML99_PRIV_CAT(ML99_PRIV_VARIADICS_GET_, i)
#define ML99_VARIADICS_TAIL(...)      ML99_PRIV_TAIL(__VA_ARGS__)

#ifndef DOXYGEN_IGNORE

#define ML99_variadicsCount_IMPL(...)    v(ML99_VARIADICS_COUNT(__VA_ARGS__))
#define ML99_variadicsIsSingle_IMPL(...) v(ML99_VARIADICS_IS_SINGLE(__VA_ARGS__))

#define ML99_PRIV_variadicsGet_0(...) ML99_call(ML99_PRIV_variadicsGet_0, __VA_ARGS__)
#define ML99_PRIV_variadicsGet_1(...) ML99_call(ML99_PRIV_variadicsGet_1, __VA_ARGS__)
#define ML99_PRIV_variadicsGet_2(...) ML99_call(ML99_PRIV_variadicsGet_2, __VA_ARGS__)
#define ML99_PRIV_variadicsGet_3(...) ML99_call(ML99_PRIV_variadicsGet_3, __VA_ARGS__)
#define ML99_PRIV_variadicsGet_4(...) ML99_call(ML99_PRIV_variadicsGet_4, __VA_ARGS__)
#define ML99_PRIV_variadicsGet_5(...) ML99_call(ML99_PRIV_variadicsGet_5, __VA_ARGS__)
#define ML99_PRIV_variadicsGet_6(...) ML99_call(ML99_PRIV_variadicsGet_6, __VA_ARGS__)
#define ML99_PRIV_variadicsGet_7(...) ML99_call(ML99_PRIV_variadicsGet_7, __VA_ARGS__)

#define ML99_PRIV_variadicsGet_0_IMPL(...) v(ML99_VARIADICS_GET(0)(__VA_ARGS__))
#define ML99_PRIV_variadicsGet_1_IMPL(...) v(ML99_VARIADICS_GET(1)(__VA_ARGS__))
#define ML99_PRIV_variadicsGet_2_IMPL(...) v(ML99_VARIADICS_GET(2)(__VA_ARGS__))
#define ML99_PRIV_variadicsGet_3_IMPL(...) v(ML99_VARIADICS_GET(3)(__VA_ARGS__))
#define ML99_PRIV_variadicsGet_4_IMPL(...) v(ML99_VARIADICS_GET(4)(__VA_ARGS__))
#define ML99_PRIV_variadicsGet_5_IMPL(...) v(ML99_VARIADICS_GET(5)(__VA_ARGS__))
#define ML99_PRIV_variadicsGet_6_IMPL(...) v(ML99_VARIADICS_GET(6)(__VA_ARGS__))
#define ML99_PRIV_variadicsGet_7_IMPL(...) v(ML99_VARIADICS_GET(7)(__VA_ARGS__))

#define ML99_PRIV_VARIADICS_GET_0(...) ML99_PRIV_VARIADICS_GET_AUX_0(__VA_ARGS__, ~)
#define ML99_PRIV_VARIADICS_GET_1(...) ML99_PRIV_VARIADICS_GET_AUX_1(__VA_ARGS__, ~)
#define ML99_PRIV_VARIADICS_GET_2(...) ML99_PRIV_VARIADICS_GET_AUX_2(__VA_ARGS__, ~)
#define ML99_PRIV_VARIADICS_GET_3(...) ML99_PRIV_VARIADICS_GET_AUX_3(__VA_ARGS__, ~)
#define ML99_PRIV_VARIADICS_GET_4(...) ML99_PRIV_VARIADICS_GET_AUX_4(__VA_ARGS__, ~)
#define ML99_PRIV_VARIADICS_GET_5(...) ML99_PRIV_VARIADICS_GET_AUX_5(__VA_ARGS__, ~)
#define ML99_PRIV_VARIADICS_GET_6(...) ML99_PRIV_VARIADICS_GET_AUX_6(__VA_ARGS__, ~)
#define ML99_PRIV_VARIADICS_GET_7(...) ML99_PRIV_VARIADICS_GET_AUX_7(__VA_ARGS__, ~)

#define ML99_PRIV_VARIADICS_GET_AUX_0(a, ...)                             a
#define ML99_PRIV_VARIADICS_GET_AUX_1(_a, b, ...)                         b
#define ML99_PRIV_VARIADICS_GET_AUX_2(_a, _b, c, ...)                     c
#define ML99_PRIV_VARIADICS_GET_AUX_3(_a, _b, _c, d, ...)                 d
#define ML99_PRIV_VARIADICS_GET_AUX_4(_a, _b, _c, _d, e, ...)             e
#define ML99_PRIV_VARIADICS_GET_AUX_5(_a, _b, _c, _d, _e, f, ...)         f
#define ML99_PRIV_VARIADICS_GET_AUX_6(_a, _b, _c, _d, _e, _f, g, ...)     g
#define ML99_PRIV_VARIADICS_GET_AUX_7(_a, _b, _c, _d, _e, _f, _g, h, ...) h

#define ML99_variadicsTail_IMPL(...) v(ML99_VARIADICS_TAIL(__VA_ARGS__))

// ML99_variadicsForEach_IMPL {

#define ML99_variadicsForEach_IMPL(f, ...)                                                         \
    ML99_PRIV_CAT(ML99_PRIV_variadicsForEach_, ML99_VARIADICS_IS_SINGLE(__VA_ARGS__))              \
    (f, __VA_ARGS__)
#define ML99_PRIV_variadicsForEach_1(f, x) ML99_appl_IMPL(f, x)
#define ML99_PRIV_variadicsForEach_0(f, x, ...)                                                    \
    ML99_TERMS(ML99_appl_IMPL(f, x), ML99_callUneval(ML99_variadicsForEach, f, __VA_ARGS__))
// } (ML99_variadicsForEach_IMPL)

// ML99_variadicsForEachI_IMPL {

#define ML99_variadicsForEachI_IMPL(f, ...) ML99_PRIV_variadicsForEachIAux_IMPL(f, 0, __VA_ARGS__)

#define ML99_PRIV_variadicsForEachIAux_IMPL(f, i, ...)                                             \
    ML99_PRIV_CAT(ML99_PRIV_variadicsForEachI_, ML99_VARIADICS_IS_SINGLE(__VA_ARGS__))             \
    (f, i, __VA_ARGS__)

#define ML99_PRIV_variadicsForEachI_1(f, i, x) ML99_appl2_IMPL(f, x, i)
#define ML99_PRIV_variadicsForEachI_0(f, i, x, ...)                                                \
    ML99_TERMS(                                                                                    \
        ML99_appl2_IMPL(f, x, i),                                                                  \
        ML99_callUneval(ML99_PRIV_variadicsForEachIAux, f, ML99_PRIV_INC(i), __VA_ARGS__))
// } (ML99_variadicsForEachI_IMPL)

/*
 * The StackOverflow solution: <https://stackoverflow.com/a/2124385/13166656>.
 *
 * This macro supports at most 63 arguments because C99 allows implementations to handle only 127
 * parameters/arguments per macro definition/invocation (C99 | 5.2.4 Environmental limits), and
 * `ML99_PRIV_VARIADICS_COUNT_AUX` already accepts 64 arguments.
 */
// clang-format off
#define ML99_PRIV_VARIADICS_COUNT(...) \
    ML99_PRIV_VARIADICS_COUNT_AUX( \
        __VA_ARGS__, \
        63, 62, 61, 60, 59, 58, 57, 56, 55, 54, \
        53, 52, 51, 50, 49, 48, 47, 46, 45, 44, \
        43, 42, 41, 40, 39, 38, 37, 36, 35, 34, \
        33, 32, 31, 30, 29, 28, 27, 26, 25, 24, \
        23, 22, 21, 20, 19, 18, 17, 16, 15, 14, \
        13, 12, 11, 10,  9,  8,  7,  6,  5,  4, \
         3,  2,  1,  ~)

#define ML99_PRIV_VARIADICS_COUNT_AUX( \
     _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
    _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
    _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
    _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, x, ...) \
        x
// clang-format on

// Arity specifiers {

#define ML99_variadicsCount_ARITY    1
#define ML99_variadicsIsSingle_ARITY 1
#define ML99_variadicsTail_ARITY     1
#define ML99_variadicsForEach_ARITY  2
#define ML99_variadicsForEachI_ARITY 2

#define ML99_PRIV_variadicsGet_0_ARITY 1
#define ML99_PRIV_variadicsGet_1_ARITY 1
#define ML99_PRIV_variadicsGet_2_ARITY 1
#define ML99_PRIV_variadicsGet_3_ARITY 1
#define ML99_PRIV_variadicsGet_4_ARITY 1
#define ML99_PRIV_variadicsGet_5_ARITY 1
#define ML99_PRIV_variadicsGet_6_ARITY 1
#define ML99_PRIV_variadicsGet_7_ARITY 1
// } (Arity specifiers)

#endif // DOXYGEN_IGNORE

#endif // ML99_VARIADICS_H
