/**
 * @file
 * Tuples: `(x, y, z)`.
 *
 * A tuple is represented as `(x1, ..., xN)`. Tuples are a convenient way to deal with product
 * types. For example:
 *
 * [[examples/rectangle.c](https://github.com/Hirrolot/metalang99/blob/master/examples/rectangle.c)]
 * @include rectangle.c
 *
 * @note Tuples are more time and space-efficient than lists, but export less functionality; if a
 * needed function is missed, invoking #ML99_list and then manipulating with the resulting Cons-list
 * might be helpful.
 */

#ifndef ML99_TUPLE_H
#define ML99_TUPLE_H

#include <metalang99/priv/bool.h>
#include <metalang99/priv/tuple.h>
#include <metalang99/priv/util.h>

#include <metalang99/lang.h>
#include <metalang99/variadics.h>

/**
 * Transforms a sequence of arguments into `(...)`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * // (1, 2, 3)
 * ML99_tuple(v(1, 2, 3))
 * @endcode
 */
#define ML99_tuple(...) ML99_call(ML99_tuple, __VA_ARGS__)

/**
 * Transforms a sequence of arguments into `(v(...))`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * // (v(1, 2, 3))
 * ML99_tupleEval(v(1, 2, 3))
 * @endcode
 *
 * @deprecated I have seen no single use case over time. Please, [open an
 * issue](https://github.com/Hirrolot/metalang99/issues/new/choose) if you need this function.
 */
#define ML99_tupleEval(...) ML99_call(ML99_tupleEval, __VA_ARGS__)

/**
 * Untuples the tuple @p x, leaving the result unevaluated.
 *
 * If @p x is not a tuple, it emits a fatal error.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * // 1, 2, 3
 * ML99_untuple(v((1, 2, 3)))
 * @endcode
 */
#define ML99_untuple(x) ML99_call(ML99_untuple, x)

/**
 * The same as #ML99_untuple.
 *
 * @deprecated Use #ML99_untuple instead.
 */
#define ML99_untupleChecked(x) ML99_call(ML99_untupleChecked, x)

/**
 * Untuples the tuple @p x and evaluates the result.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * // 1, 2, 3
 * ML99_untupleEval(v((v(1, 2, 3))))
 * @endcode
 *
 * @deprecated For the same reason as #ML99_tupleEval.
 */
#define ML99_untupleEval(x) ML99_call(ML99_untupleEval, x)

/**
 * Tests whether @p x is inside parentheses or not.
 *
 * The preconditions are the same as of #ML99_isUntuple.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * // 0
 * ML99_isTuple(v(123))
 *
 * // 1
 * ML99_isTuple(v((123)))
 * @endcode
 */
#define ML99_isTuple(x) ML99_call(ML99_isTuple, x)

/**
 * The inverse of #ML99_isTuple.
 *
 * @p x must be either of these forms:
 *  - `(...)` (reported as non-untupled)
 *  - `(...) (...) ...` (reported as untupled)
 *  - anything else not beginning with `(...)` (reported as untupled)
 *
 * For example (respectively):
 *  - `(~, ~, ~)` (non-untupled)
 *  - `(~, ~, ~) (~, ~, ~)` or `(~, ~, ~) (~, ~, ~) abc` (untupled)
 *  - `123` or `123 (~, ~, ~)` (untupled)
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * // 1
 * ML99_isUntuple(v(123))
 *
 * // 0
 * ML99_isUntuple(v((123)))
 *
 * // 1
 * ML99_isUntuple(v((123) (456) (789)))
 * @endcode
 */
#define ML99_isUntuple(x) ML99_call(ML99_isUntuple, x)

/**
 * Computes the count of items in the tuple @p x.
 *
 * At most 63 items can be contained in @p x.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * // 3
 * ML99_tupleCount(v((~, ~, ~)))
 *
 * // 1
 * ML99_tupleCount(v(()))
 * @endcode
 */
#define ML99_tupleCount(x) ML99_call(ML99_tupleCount, x)

/**
 * Tells if the tuple @p x contains only one item or not.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * // 1
 * ML99_tupleIsSingle(v((~)))
 *
 * // 0
 * ML99_tupleIsSingle(v((~, ~, ~)))
 * @endcode
 */
#define ML99_tupleIsSingle(x) ML99_call(ML99_tupleIsSingle, x)

/**
 * Expands to a metafunction extracting the @p i -indexed element of a tuple.
 *
 * @p i can range from 0 to 7, inclusively.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * // 2
 * ML99_tupleGet(1)(v((1, 2, 3)))
 * @endcode
 */
#define ML99_tupleGet(i) ML99_PRIV_CAT(ML99_PRIV_tupleGet_, i)

/**
 * Extracts the tuple's tail.
 *
 * @p x must contain at least two elements.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * // 2, 3
 * ML99_tupleTail(v((1, 2, 3)))
 * @endcode
 */
#define ML99_tupleTail(x) ML99_call(ML99_tupleTail, x)

/**
 * Appends provided variadic arguments to the tuple @p x.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * // (1, 2, 3)
 * ML99_tupleAppend(ML99_tuple(v(1)), v(2, 3))
 * @endcode
 */
#define ML99_tupleAppend(x, ...) ML99_call(ML99_tupleAppend, x, __VA_ARGS__)

/**
 * Prepends provided variadic arguments to the tuple @p x.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * // (1, 2, 3)
 * ML99_tuplePrepend(ML99_tuple(v(3)), v(1, 2))
 * @endcode
 */
#define ML99_tuplePrepend(x, ...) ML99_call(ML99_tuplePrepend, x, __VA_ARGS__)

/**
 * A shortcut for `ML99_variadicsForEach(f, ML99_untuple(x))`.
 */
#define ML99_tupleForEach(f, x) ML99_call(ML99_tupleForEach, f, x)

/**
 * A shortcut for `ML99_variadicsForEachI(f, ML99_untuple(x))`.
 */
#define ML99_tupleForEachI(f, x) ML99_call(ML99_tupleForEachI, f, x)

/**
 * Emits a fatal error if @p x is not a tuple, otherwise results in emptiness.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/tuple.h>
 *
 * #define F_IMPL(x) ML99_TERMS(ML99_assertIsTuple(v(x)), ML99_untuple(v(x)))
 *
 * // 1, 2, 3
 * ML99_call(F, v((1, 2, 3)))
 *
 * // A compile-time tuple mismatch error.
 * ML99_call(F, v(123))
 * @endcode
 */
#define ML99_assertIsTuple(x) ML99_call(ML99_assertIsTuple, x)

#define ML99_TUPLE(...)            (__VA_ARGS__)
#define ML99_UNTUPLE(x)            ML99_PRIV_EXPAND x
#define ML99_IS_TUPLE(x)           ML99_PRIV_IS_TUPLE(x)
#define ML99_IS_UNTUPLE(x)         ML99_PRIV_IS_UNTUPLE(x)
#define ML99_TUPLE_COUNT(x)        ML99_VARIADICS_COUNT(ML99_UNTUPLE(x))
#define ML99_TUPLE_IS_SINGLE(x)    ML99_VARIADICS_IS_SINGLE(ML99_UNTUPLE(x))
#define ML99_TUPLE_GET(i)          ML99_PRIV_CAT(ML99_PRIV_TUPLE_GET_, i)
#define ML99_TUPLE_TAIL(x)         ML99_VARIADICS_TAIL(ML99_UNTUPLE(x))
#define ML99_TUPLE_APPEND(x, ...)  (ML99_UNTUPLE(x), __VA_ARGS__)
#define ML99_TUPLE_PREPEND(x, ...) (__VA_ARGS__, ML99_UNTUPLE(x))

#ifndef DOXYGEN_IGNORE

#define ML99_tuple_IMPL(...)     v(ML99_TUPLE(__VA_ARGS__))
#define ML99_tupleEval_IMPL(...) v((v(__VA_ARGS__)))
#define ML99_untuple_IMPL(x)                                                                       \
    ML99_PRIV_IF(ML99_IS_TUPLE(x), ML99_PRIV_UNTUPLE_TERM, ML99_PRIV_NOT_TUPLE_ERROR)(x)
#define ML99_untupleChecked_IMPL(x) ML99_untuple_IMPL(x)
#define ML99_untupleEval_IMPL(x)    ML99_PRIV_EXPAND x
#define ML99_isTuple_IMPL(x)        v(ML99_IS_TUPLE(x))
#define ML99_isUntuple_IMPL(x)      v(ML99_IS_UNTUPLE(x))
#define ML99_tupleCount_IMPL(x)     v(ML99_TUPLE_COUNT(x))
#define ML99_tupleIsSingle_IMPL(x)  v(ML99_TUPLE_IS_SINGLE(x))

#define ML99_PRIV_UNTUPLE_TERM(x) v(ML99_UNTUPLE(x))

#define ML99_PRIV_tupleGet_0(x) ML99_call(ML99_PRIV_tupleGet_0, x)
#define ML99_PRIV_tupleGet_1(x) ML99_call(ML99_PRIV_tupleGet_1, x)
#define ML99_PRIV_tupleGet_2(x) ML99_call(ML99_PRIV_tupleGet_2, x)
#define ML99_PRIV_tupleGet_3(x) ML99_call(ML99_PRIV_tupleGet_3, x)
#define ML99_PRIV_tupleGet_4(x) ML99_call(ML99_PRIV_tupleGet_4, x)
#define ML99_PRIV_tupleGet_5(x) ML99_call(ML99_PRIV_tupleGet_5, x)
#define ML99_PRIV_tupleGet_6(x) ML99_call(ML99_PRIV_tupleGet_6, x)
#define ML99_PRIV_tupleGet_7(x) ML99_call(ML99_PRIV_tupleGet_7, x)

#define ML99_PRIV_tupleGet_0_IMPL(x) v(ML99_TUPLE_GET(0)(x))
#define ML99_PRIV_tupleGet_1_IMPL(x) v(ML99_TUPLE_GET(1)(x))
#define ML99_PRIV_tupleGet_2_IMPL(x) v(ML99_TUPLE_GET(2)(x))
#define ML99_PRIV_tupleGet_3_IMPL(x) v(ML99_TUPLE_GET(3)(x))
#define ML99_PRIV_tupleGet_4_IMPL(x) v(ML99_TUPLE_GET(4)(x))
#define ML99_PRIV_tupleGet_5_IMPL(x) v(ML99_TUPLE_GET(5)(x))
#define ML99_PRIV_tupleGet_6_IMPL(x) v(ML99_TUPLE_GET(6)(x))
#define ML99_PRIV_tupleGet_7_IMPL(x) v(ML99_TUPLE_GET(7)(x))

#define ML99_PRIV_TUPLE_GET_0(x) ML99_VARIADICS_GET(0)(ML99_UNTUPLE(x))
#define ML99_PRIV_TUPLE_GET_1(x) ML99_VARIADICS_GET(1)(ML99_UNTUPLE(x))
#define ML99_PRIV_TUPLE_GET_2(x) ML99_VARIADICS_GET(2)(ML99_UNTUPLE(x))
#define ML99_PRIV_TUPLE_GET_3(x) ML99_VARIADICS_GET(3)(ML99_UNTUPLE(x))
#define ML99_PRIV_TUPLE_GET_4(x) ML99_VARIADICS_GET(4)(ML99_UNTUPLE(x))
#define ML99_PRIV_TUPLE_GET_5(x) ML99_VARIADICS_GET(5)(ML99_UNTUPLE(x))
#define ML99_PRIV_TUPLE_GET_6(x) ML99_VARIADICS_GET(6)(ML99_UNTUPLE(x))
#define ML99_PRIV_TUPLE_GET_7(x) ML99_VARIADICS_GET(7)(ML99_UNTUPLE(x))

#define ML99_tupleTail_IMPL(x) v(ML99_TUPLE_TAIL(x))

#define ML99_tupleAppend_IMPL(x, ...)  v(ML99_TUPLE_APPEND(x, __VA_ARGS__))
#define ML99_tuplePrepend_IMPL(x, ...) v(ML99_TUPLE_PREPEND(x, __VA_ARGS__))
#define ML99_tupleForEach_IMPL(f, x)   ML99_variadicsForEach_IMPL(f, ML99_UNTUPLE(x))
#define ML99_tupleForEachI_IMPL(f, x)  ML99_variadicsForEachI_IMPL(f, ML99_UNTUPLE(x))

#define ML99_assertIsTuple_IMPL(x)                                                                 \
    ML99_PRIV_IF(ML99_IS_UNTUPLE(x), ML99_PRIV_NOT_TUPLE_ERROR(x), v(ML99_PRIV_EMPTY()))

// clang-format off
#define ML99_PRIV_NOT_TUPLE_ERROR(x) \
    ML99_PRIV_IF( \
        ML99_PRIV_IS_DOUBLE_TUPLE_BEGINNING(x), \
        ML99_fatal(ML99_assertIsTuple, x must be (x1, ..., xN), did you miss a comma?), \
        ML99_fatal(ML99_assertIsTuple, x must be (x1, ..., xN)))
// clang-format on

// Arity specifiers {

#define ML99_tuple_ARITY          1
#define ML99_tupleEval_ARITY      1
#define ML99_untuple_ARITY        1
#define ML99_untupleChecked_ARITY 1
#define ML99_untupleEval_ARITY    1
#define ML99_isTuple_ARITY        1
#define ML99_isUntuple_ARITY      1
#define ML99_tupleCount_ARITY     1
#define ML99_tupleIsSingle_ARITY  1
#define ML99_tupleTail_ARITY      1
#define ML99_tupleAppend_ARITY    2
#define ML99_tuplePrepend_ARITY   2
#define ML99_tupleForEach_ARITY   2
#define ML99_tupleForEachI_ARITY  2
#define ML99_assertIsTuple_ARITY  1

#define ML99_PRIV_tupleGet_0_ARITY 1
#define ML99_PRIV_tupleGet_1_ARITY 1
#define ML99_PRIV_tupleGet_2_ARITY 1
#define ML99_PRIV_tupleGet_3_ARITY 1
#define ML99_PRIV_tupleGet_4_ARITY 1
#define ML99_PRIV_tupleGet_5_ARITY 1
#define ML99_PRIV_tupleGet_6_ARITY 1
#define ML99_PRIV_tupleGet_7_ARITY 1
// } (Arity specifiers)

#endif // DOXYGEN_IGNORE

#endif // ML99_TUPLE_H
