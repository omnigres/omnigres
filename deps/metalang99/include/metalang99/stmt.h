/**
 * @file
 * Statement chaining.
 *
 * This module exports a bunch of so-called _statement chaining macros_: they expect a statement
 * right after their invocation, and moreover, an invocation of such a macro with a statement
 * afterwards altogether form a single statement.
 *
 * How can this be helpful? Imagine you are writing a macro with the following syntax:
 *
 * @code
 * MY_MACRO(...) { bla bla bla }
 * @endcode
 *
 * Then `MY_MACRO` must expand to a _statement prefix_, i.e., something that expects a statement
 * after itself. One possible solution is to make `MY_MACRO` expand to a sequence of statement
 * chaining macros like this:
 *
 * @code
 * #define MY_MACRO(...) \
 *     ML99_INTRODUCE_VAR_TO_STMT(int x = 5) \
 *         ML99_CHAIN_EXPR_STMT(printf("%d\n", x)) \
 *             // and so on...
 * @endcode
 *
 * Here #ML99_INTRODUCE_VAR_TO_STMT accepts the statement formed by #ML99_CHAIN_EXPR_STMT, which, in
 * turn, accepts the next statement and so on, until a caller of `MY_MACRO` specifies the final
 * statement, thus completing the chain.
 *
 * @see https://www.chiark.greenend.org.uk/~sgtatham/mp/ for a more involved explanation.
 */

#ifndef ML99_STMT_H
#define ML99_STMT_H

#include <metalang99/util.h>

/**
 * A statement chaining macro that introduces several variable definitions to a statement right
 * after its invocation.
 *
 * Variable definitions must be specified as in the first clause of the for-loop.
 *
 * Top-level `break`/`continue` inside a user-provided statement are prohibited.
 *
 * # Example
 *
 * @code
 * #include <metalang99/stmt.h>
 *
 * for (int i = 0; i < 10; i++)
 *     ML99_INTRODUCE_VAR_TO_STMT(double x = 5.0, y = 7.0)
 *         if (i % 2 == 0)
 *             printf("i = %d, x = %f, y = %f\n", i, x, y);
 * @endcode
 */
#define ML99_INTRODUCE_VAR_TO_STMT(...) ML99_PRIV_INTRODUCE_VAR_TO_STMT_INNER(__VA_ARGS__)

/**
 * The same as #ML99_INTRODUCE_VAR_TO_STMT but deals with a single non-`NULL` pointer.
 *
 * In comparison with #ML99_INTRODUCE_VAR_TO_STMT, this macro generates a little less code. It
 * introduces a pointer to @p ty identified by @p name and initialised to @p init.
 *
 * Top-level `break`/`continue` inside a user-provided statement are prohibited.
 *
 * # Example
 *
 * @code
 * #include <metalang99/stmt.h>
 *
 * double x = 5.0, y = 7.0;
 *
 * for (int i = 0; i < 10; i++)
 *     ML99_INTRODUCE_NON_NULL_PTR_TO_STMT(double, x_ptr, &x)
 *         ML99_INTRODUCE_NON_NULL_PTR_TO_STMT(double, y_ptr, &y)
 *             printf("i = %d, x = %f, y = %f\n", i, *x_ptr, *y_ptr);
 * @endcode
 *
 * @note Unlike #ML99_INTRODUCE_VAR_TO_STMT, the generated pointer is guaranteed to be used at least
 * once, meaning that you do not need to suppress the unused variable warning.
 * @note @p init is guaranteed to be executed only once.
 */
#define ML99_INTRODUCE_NON_NULL_PTR_TO_STMT(ty, name, init)                                        \
    ML99_PRIV_SHADOWS(for (ty *name = (init); name != 0; name = 0))

/**
 * A statement chaining macro that executes an expression statement derived from @p expr right
 * before the next statement.
 *
 * Top-level `break`/`continue` inside a user-provided statement are prohibited.
 *
 * # Example
 *
 * @code
 * #include <metalang99/stmt.h>
 *
 * int x;
 *
 * for(;;)
 *     ML99_CHAIN_EXPR_STMT(x = 5)
 *         ML99_CHAIN_EXPR_STMT(printf("%d\n", x))
 *             puts("abc");
 * @endcode
 */
#define ML99_CHAIN_EXPR_STMT(expr)                                                                 \
    ML99_PRIV_SHADOWS(for (int ml99_priv_expr_stmt_break = ((expr), 0);                            \
                           ml99_priv_expr_stmt_break != 1;                                         \
                           ml99_priv_expr_stmt_break = 1))

/**
 * The same as #ML99_CHAIN_EXPR_STMT but executes @p expr **after** the next statement.
 */
#define ML99_CHAIN_EXPR_STMT_AFTER(expr)                                                           \
    ML99_PRIV_SHADOWS(for (int ml99_priv_expr_stmt_after_break = 0;                                \
                           ml99_priv_expr_stmt_after_break != 1;                                   \
                           ((expr), ml99_priv_expr_stmt_after_break = 1)))

/**
 * A statement chaining macro that suppresses the "unused X" warning right before a statement after
 * its invocation.
 *
 * Top-level `break`/`continue` inside a user-provided statement are prohibited.
 *
 * # Example
 *
 * @code
 * #include <metalang99/stmt.h>
 *
 * int x, y;
 *
 * for(;;)
 *     ML99_SUPPRESS_UNUSED_BEFORE_STMT(x)
 *         ML99_SUPPRESS_UNUSED_BEFORE_STMT(y)
 *             puts("abc");
 * @endcode
 *
 * @deprecated Use `ML99_CHAIN_EXPR_STMT((void)expr)` instead.
 */
#define ML99_SUPPRESS_UNUSED_BEFORE_STMT(expr) ML99_CHAIN_EXPR_STMT((void)expr)

#ifndef DOXYGEN_IGNORE

// See <https://github.com/Hirrolot/metalang99/issues/25>.
#ifdef __cplusplus
#define ML99_PRIV_INTRODUCE_VAR_TO_STMT_INNER(...)                                                 \
    ML99_PRIV_SHADOWS(for (__VA_ARGS__,                                                            \
                           *ml99_priv_break_arr[] = {0, 0},                                        \
                           **ml99_priv_break = &ml99_priv_break_arr[0];                            \
                           ml99_priv_break == &ml99_priv_break_arr[0];                             \
                           ml99_priv_break++))
#else
#define ML99_PRIV_INTRODUCE_VAR_TO_STMT_INNER(...)                                                 \
    ML99_PRIV_SHADOWS(for (__VA_ARGS__, *ml99_priv_break = (void *)0;                              \
                           ml99_priv_break != (void *)1;                                           \
                           ml99_priv_break = (void *)1))
#endif

#define ML99_PRIV_SHADOWS(...)                                                                     \
    ML99_CLANG_PRAGMA("clang diagnostic push")                                                     \
    ML99_CLANG_PRAGMA("clang diagnostic ignored \"-Wshadow\"")                                     \
    __VA_ARGS__                                                                                    \
    ML99_CLANG_PRAGMA("clang diagnostic pop")

#endif // DOXYGEN_IGNORE

#endif // ML99_STMT_H
