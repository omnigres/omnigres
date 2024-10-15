/**
 * @file
 * A choice type with two cases.
 */

#ifndef ML99_EITHER_H
#define ML99_EITHER_H

#include <metalang99/priv/util.h>

#include <metalang99/bool.h>
#include <metalang99/choice.h>
#include <metalang99/ident.h>

/**
 * The left value @p x.
 */
#define ML99_left(x) ML99_call(ML99_left, x)

/**
 * The right value @p x.
 */
#define ML99_right(x) ML99_call(ML99_right, x)

/**
 * `ML99_true()` if @p either contains a left value, otherwise `ML99_false()`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/either.h>
 *
 * // 1
 * ML99_isLeft(ML99_left(v(123)))
 *
 * // 0
 * ML99_isLeft(ML99_right(v(123)))
 * @endcode
 */
#define ML99_isLeft(either) ML99_call(ML99_isLeft, either)

/**
 * The inverse of #ML99_isLeft.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/either.h>
 *
 * // 1
 * ML99_isRight(ML99_right(v(123)))
 *
 * // 0
 * ML99_isRight(ML99_left(v(123)))
 * @endcode
 */
#define ML99_isRight(either) ML99_call(ML99_isRight, either)

/**
 * Tests @p either and @p other for equality.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/either.h>
 * #include <metalang99/nat.h>
 *
 * // 1
 * ML99_eitherEq(v(ML99_natEq), ML99_left(v(123)), ML99_left(v(123)))
 *
 * // 0
 * ML99_eitherEq(v(ML99_natEq), ML99_right(v(123)), ML99_left(v(8)))
 *
 * // 0
 * ML99_eitherEq(v(ML99_natEq), ML99_right(v(123)), ML99_left(v(123)))
 * @endcode
 */
#define ML99_eitherEq(cmp, either, other) ML99_call(ML99_eitherEq, cmp, either, other)

/**
 * Returns the left value on `ML99_left(x)` or emits a fatal error on `ML99_right(y)`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/either.h>
 *
 * // 123
 * ML99_unwrapLeft(ML99_left(v(123)))
 *
 * // Emits a fatal error.
 * ML99_unwrapLeft(ML99_right(v(123)))
 * @endcode
 */
#define ML99_unwrapLeft(either) ML99_call(ML99_unwrapLeft, either)

/**
 * The inverse of #ML99_unwrapLeft.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/either.h>
 *
 * // 123
 * ML99_unwrapRight(ML99_right(v(123)))
 *
 * // Emits a fatal error.
 * ML99_unwrapRight(ML99_left(v(123)))
 * @endcode
 */
#define ML99_unwrapRight(either) ML99_call(ML99_unwrapRight, either)

#define ML99_LEFT(x)          ML99_CHOICE(left, x)
#define ML99_RIGHT(x)         ML99_CHOICE(right, x)
#define ML99_IS_LEFT(either)  ML99_PRIV_IS_LEFT(either)
#define ML99_IS_RIGHT(either) ML99_NOT(ML99_IS_LEFT(either))

#ifndef DOXYGEN_IGNORE

#define ML99_left_IMPL(x)  v(ML99_LEFT(x))
#define ML99_right_IMPL(x) v(ML99_RIGHT(x))

#define ML99_isLeft_IMPL(either)  v(ML99_IS_LEFT(either))
#define ML99_isRight_IMPL(either) v(ML99_IS_RIGHT(either))

// ML99_eitherEq_IMPL {

#define ML99_eitherEq_IMPL(cmp, either, other)                                                     \
    ML99_matchWithArgs_IMPL(either, ML99_PRIV_eitherEq_, cmp, other)

#define ML99_PRIV_eitherEq_left_IMPL(x, cmp, other)                                                \
    ML99_matchWithArgs_IMPL(other, ML99_PRIV_eitherEq_left_, cmp, x)
#define ML99_PRIV_eitherEq_right_IMPL(x, cmp, other)                                               \
    ML99_matchWithArgs_IMPL(other, ML99_PRIV_eitherEq_right_, cmp, x)

#define ML99_PRIV_eitherEq_left_left_IMPL(y, cmp, x) ML99_appl2_IMPL(cmp, x, y)
#define ML99_PRIV_eitherEq_left_right_IMPL           ML99_false_IMPL

#define ML99_PRIV_eitherEq_right_left_IMPL             ML99_false_IMPL
#define ML99_PRIV_eitherEq_right_right_IMPL(y, cmp, x) ML99_appl2_IMPL(cmp, x, y)
// } (ML99_eitherEq_IMPL)

#define ML99_unwrapLeft_IMPL(either)      ML99_match_IMPL(either, ML99_PRIV_unwrapLeft_)
#define ML99_PRIV_unwrapLeft_left_IMPL(x) v(x)
#define ML99_PRIV_unwrapLeft_right_IMPL(_x)                                                        \
    ML99_fatal(ML99_unwrapLeft, expected ML99_left but found ML99_right)

#define ML99_unwrapRight_IMPL(either) ML99_match_IMPL(either, ML99_PRIV_unwrapRight_)
#define ML99_PRIV_unwrapRight_left_IMPL(_x)                                                        \
    ML99_fatal(ML99_unwrapRight, expected ML99_right but found ML99_left)
#define ML99_PRIV_unwrapRight_right_IMPL(x) v(x)

#define ML99_PRIV_IS_LEFT(either) ML99_DETECT_IDENT(ML99_PRIV_IS_LEFT_, ML99_CHOICE_TAG(either))
#define ML99_PRIV_IS_LEFT_left    ()

// Arity specifiers {

#define ML99_left_ARITY        1
#define ML99_right_ARITY       1
#define ML99_isLeft_ARITY      1
#define ML99_isRight_ARITY     1
#define ML99_eitherEq_ARITY    3
#define ML99_unwrapLeft_ARITY  1
#define ML99_unwrapRight_ARITY 1
// } (Arity specifiers)

#endif // DOXYGEN_IGNORE

#endif // ML99_EITHER_H
