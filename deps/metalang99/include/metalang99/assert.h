/**
 * @file
 * Static assertions.
 *
 * For the sake of convenience, this header automatically includes `metalang99/bool.h`.
 *
 * @note [C99] Any of the following assertion macros must **not** appear on the same line number
 * twice with itself as well as with any other Metalang99 assertion macro.
 * @note [C11] The following assertion macros expand to `_Static_assert` and, therefore, can be used
 * on the same line twice.
 */

#ifndef ML99_ASSERT_H
#define ML99_ASSERT_H

#include <metalang99/priv/compiler_specific.h>

#include <metalang99/bool.h>
#include <metalang99/lang.h>

/**
 * The same as #ML99_ASSERT but results in a Metalang99 term.
 *
 * It can be used inside other Metalang99-compliant macros, unlike #ML99_ASSERT, which uses
 * #ML99_EVAL internally.
 */
#define ML99_assert(expr) ML99_call(ML99_assert, expr)

/**
 * Like #ML99_assert but compares @p lhs with @p rhs for equality (`==`).
 */
#define ML99_assertEq(lhs, rhs) ML99_call(ML99_assertEq, lhs, rhs)

/**
 * Asserts `ML99_EVAL(expr)` at compile-time.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/assert.h>
 *
 * ML99_ASSERT(v(123 == 123));
 * @endcode
 */
#define ML99_ASSERT(expr) ML99_ASSERT_EQ(expr, ML99_true())

/**
 * Asserts `ML99_EVAL(lhs) == ML99_EVAL(rhs)` at compile-time.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/assert.h>
 *
 * ML99_ASSERT_EQ(v(123), v(123));
 * @endcode
 */
#define ML99_ASSERT_EQ(lhs, rhs) ML99_ASSERT_UNEVAL((ML99_EVAL(lhs)) == (ML99_EVAL(rhs)))

/**
 * Asserts the C constant expression @p expr;
 * [static_assert](https://en.cppreference.com/w/c/error/static_assert) in pure C99.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/assert.h>
 *
 * ML99_ASSERT_UNEVAL(123 == 123);
 * @endcode
 */
#define ML99_ASSERT_UNEVAL(expr) ML99_PRIV_ASSERT_UNEVAL_INNER(expr)

/**
 * Asserts that `ML99_EVAL(expr)` is emptiness.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/assert.h>
 *
 * // Passes:
 * ML99_ASSERT_EMPTY(v());
 *
 * // Fails:
 * ML99_ASSERT_EMPTY(v(123));
 * @endcode
 */
#define ML99_ASSERT_EMPTY(expr) ML99_ASSERT_EMPTY_UNEVAL(ML99_EVAL(expr))

/**
 * Asserts that @p expr is emptiness.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/assert.h>
 *
 * // Passes:
 * ML99_ASSERT_EMPTY_UNEVAL();
 *
 * // Fails:
 * ML99_ASSERT_EMPTY_UNEVAL(123);
 * @endcode
 */
#define ML99_ASSERT_EMPTY_UNEVAL(expr)                                                             \
    ML99_ASSERT_UNEVAL(ML99_PRIV_CAT(ML99_PRIV_ASSERT_EMPTY_, expr))

#ifndef DOXYGEN_IGNORE

#define ML99_assert_IMPL(expr)       v(ML99_ASSERT_UNEVAL(expr))
#define ML99_assertEq_IMPL(lhs, rhs) v(ML99_ASSERT_UNEVAL((lhs) == (rhs)))

#ifdef ML99_PRIV_C11_STATIC_ASSERT_AVAILABLE
#define ML99_PRIV_ASSERT_UNEVAL_INNER(expr) _Static_assert((expr), "Metalang99 assertion failed")
#else
// How to imitate static assertions in C99: <https://stackoverflow.com/a/3385694/13166656>.
#define ML99_PRIV_ASSERT_UNEVAL_INNER(expr)                                                        \
    static const char ML99_PRIV_CAT(                                                               \
        ml99_assert_,                                                                              \
        __LINE__)[(expr) ? 1 : -1] ML99_PRIV_COMPILER_ATTR_UNUSED = {0}
#endif

#define ML99_PRIV_ASSERT_EMPTY_ 1

// Arity specifiers {

#define ML99_assert_ARITY   1
#define ML99_assertEq_ARITY 2
// } (Arity specifiers)

#endif // DOXYGEN_IGNORE

#endif // ML99_ASSERT_H
