/**
 * @file
 * Utilitary stuff.
 */

#ifndef ML99_UTIL_H
#define ML99_UTIL_H

#include <metalang99/priv/util.h>

#include <metalang99/ident.h> // For backwards compatibility.
#include <metalang99/lang.h>

/**
 * Concatenates @p a with @p b and evaluates the result.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * #define ABC123 v(Billie Jean)
 *
 * // Billie Jean
 * ML99_catEval(v(ABC), v(123))
 *
 * // ERROR: 123ABC is not a valid Metalang99 term.
 * ML99_catEval(v(123), v(ABC))
 * @endcode
 *
 * @deprecated I have seen no single use case over time. Please, [open an
 * issue](https://github.com/Hirrolot/metalang99/issues/new/choose) if you need this function.
 */
#define ML99_catEval(a, b) ML99_call(ML99_catEval, a, b)

/**
 * Concatenates @p a with @p b, leaving the result unevaluated.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * #define ABC123 Billie Jean
 *
 * // Billie Jean
 * ML99_cat(v(ABC), v(123))
 *
 * // 123ABC
 * ML99_cat(v(123), v(ABC))
 * @endcode
 */
#define ML99_cat(a, b) ML99_call(ML99_cat, a, b)

/**
 * The same as #ML99_cat but deals with 3 parameters.
 */
#define ML99_cat3(a, b, c) ML99_call(ML99_cat3, a, b, c)

/**
 * The same as #ML99_cat but deals with 4 parameters.
 */
#define ML99_cat4(a, b, c, d) ML99_call(ML99_cat4, a, b, c, d)

/**
 * Stringifies provided arguments.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * // "Billie Jean"
 * ML99_stringify(v(Billie Jean))
 * @endcode
 */
#define ML99_stringify(...) ML99_call(ML99_stringify, __VA_ARGS__)

/**
 * Evaluates to nothing.
 */
#define ML99_empty(...) ML99_callUneval(ML99_empty, )

/**
 * Evaluates to its arguments.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * // 1, 2, 3
 * ML99_id(v(1, 2, 3))
 * @endcode
 */
#define ML99_id(...) ML99_call(ML99_id, __VA_ARGS__)

/**
 * Evaluates to @p x, skipping @p a.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * // 123
 * ML99_const(v(123), v(5))
 * @endcode
 */
#define ML99_const(x, a) ML99_call(ML99_const, x, a)

/**
 * Reverses the order of arguments of the binary function @p f.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * // ABC123
 * ML99_appl2(ML99_flip(v(ML99_catUnevaluated)), v(123), v(ABC))
 * @endcode
 */
#define ML99_flip(f) ML99_call(ML99_flip, f)

/**
 * Accepts terms and evaluates them with the space-separator.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * // 1 2 3
 * ML99_uncomma(ML99_QUOTE(v(1), v(2), v(3)))
 * @endcode
 */
#define ML99_uncomma(...) ML99_call(ML99_uncomma, __VA_ARGS__)

/**
 * Turns @p f into a Metalang99-compliant metafunction with the arity of 1, which can be then called
 * by #ML99_appl.
 *
 * @p f can be any C function or function-like macro.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 * #include <metalang99/variadics.h>
 *
 * #define F(x) @x
 *
 * // @1 @2 @3
 * ML99_variadicsForEach(ML99_reify(v(F)), v(1, 2, 3))
 * @endcode
 *
 * Without #ML99_reify, you would need to write some additional boilerplate:
 *
 * @code
 * #define F_IMPL(x) v(@x)
 * #define F_ARITY   1
 * @endcode
 */
#define ML99_reify(f) ML99_call(ML99_reify, f)

/**
 * Indicates not yet implemented functionality of the macro @p f.
 *
 * #ML99_todo is the same as #ML99_unimplemented except that the former conveys an intent that the
 * functionality is to be implemented later but #ML99_unimplemented makes no such claims.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * // A not-yet implemented error.
 * ML99_todo(v(F))
 * @endcode
 *
 * @see [Rust's std::todo\!](https://doc.rust-lang.org/core/macro.todo.html) (thanks for the idea!)
 */
#define ML99_todo(f) ML99_call(ML99_todo, f)

/**
 * The same as #ML99_todo but emits a caller-supplied message.
 *
 * @p message must be a string literal.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * // A not-yet-implemented error.
 * ML99_todoWithMsg(v(F), v("your message"))
 * @endcode
 */
#define ML99_todoWithMsg(f, message) ML99_call(ML99_todoWithMsg, f, message)

/**
 * Indicates unimplemented functionality of the macro @p f.
 *
 * #ML99_unimplemented is the same as #ML99_todo except that the latter conveys an intent that the
 * functionality is to be implemented later but #ML99_unimplemented makes no such claims.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * // A not-implemented error.
 * ML99_unimplemented(v(F))
 * @endcode
 *
 * @see [Rust's std::unimplemented\!](https://doc.rust-lang.org/core/macro.unimplemented.html)
 * (thanks for the idea!)
 */
#define ML99_unimplemented(f) ML99_call(ML99_unimplemented, f)

/**
 * The same as #ML99_unimplemented but emits a caller-supplied message.
 *
 * @p message must be a string literal.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * // A not-implemented error.
 * ML99_unimplementedWithMsg(v(F), v("your message"))
 * @endcode
 */
#define ML99_unimplementedWithMsg(f, message) ML99_call(ML99_unimplementedWithMsg, f, message)

#ifdef __COUNTER__

/**
 * Generates a unique identifier @p id in the namespace @p prefix.
 *
 * Let `FOO` be the name of an enclosing macro. Then `FOO_` must be specified for @p prefix, and @p
 * id should be given any meaningful name (this makes debugging easier).
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * #define FOO(...) FOO_NAMED(ML99_GEN_SYM(FOO_, x), __VA_ARGS__)
 * #define FOO_NAMED(x_sym, ...) \
 *      do { int x_sym = 5; __VA_ARGS__ } while (0)
 *
 * // `x` here will not conflict with the `x` inside `FOO`.
 * FOO({
 *     int x = 7;
 *     printf("x is %d\n", x); // x is 7
 * });
 * @endcode
 *
 * @note Two identical calls to #ML99_GEN_SYM will yield different identifiers, therefore, to refer
 * to the result later, you must save it in an auxiliary macro's parameter, as shown in the example
 * above.
 * @note #ML99_GEN_SYM is defined only if `__COUNTER__` is defined, which must be a macro yielding
 * integral literals starting from 0 incremented by 1 each time it is called. Currently, it is
 * supported at least by Clang, GCC, TCC, and MSVC.
 * @see https://en.wikipedia.org/wiki/Hygienic_macro
 */
#define ML99_GEN_SYM(prefix, id) ML99_CAT4(prefix, id, _, __COUNTER__)

#endif // __COUNTER__

/**
 * Forces a caller to put a trailing semicolon.
 *
 * It is useful when defining macros, to make them formatted as complete statements.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * #define MY_MACRO(fn_name, val_ty, val) \
 *     inline static val_ty fn_name(void) { return val; } \
 *     ML99_TRAILING_SEMICOLON()
 *
 * // Defines a function that always returns 0.
 * MY_MACRO(zero, int, 0);
 * @endcode
 *
 * @note #ML99_TRAILING_SEMICOLON is to be used outside of functions: unlike the `do { ... } while
 * (0)` idiom, this macro expands to a C declaration.
 */
#define ML99_TRAILING_SEMICOLON(...) struct ml99_priv_trailing_semicolon

/**
 * Concatenates @p a with @p b as-is, without expanding them.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * // This macro will not be expanded.
 * #define ABC 7
 *
 * // ABC123
 * ML99_CAT_PRIMITIVE(ABC, 123)
 * @endcode
 */
#define ML99_CAT_PRIMITIVE(a, b) a##b

/**
 * The same as #ML99_CAT_PRIMITIVE but deals with 3 parameters.
 */
#define ML99_CAT3_PRIMITIVE(a, b, c) a##b##c

/**
 * The same as #ML99_CAT_PRIMITIVE but deals with 4 parameters.
 */
#define ML99_CAT4_PRIMITIVE(a, b, c, d) a##b##c##d

/**
 * Stringifies @p x as-is, without expanding it.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/util.h>
 *
 * // This macro will not be expanded.
 * #define ABC 7
 *
 * // "ABC"
 * ML99_STRINGIFY_PRIMITIVE(ABC)
 * @endcode
 */
#define ML99_STRINGIFY_PRIMITIVE(...) #__VA_ARGS__

/**
 * Expands to an opening parenthesis (`(`).
 *
 * This is helpful when you want to delay macro arguments passing: just type `BAR ML99_LPAREN()
 * initial args...` at the end of some macro `FOO` and complete the invocation of `BAR` with
 * `the rest of args...)` in future.
 *
 * This macro consumes all its arguments.
 *
 * @deprecated This macro results in code that is difficult to reason about.
 */
#define ML99_LPAREN(...) (

/**
 * The same as #ML99_LPAREN but emits a closing parenthesis.
 *
 * @deprecated For the same reason as #ML99_LPAREN.
 */
#define ML99_RPAREN(...) )

/**
 * Expands to a single comma, consuming all arguments.
 */
#define ML99_COMMA(...) ,

/**
 * If you are compiling on GCC, this macro expands to `_Pragma(str)`, otherwise to emptiness.
 */
#define ML99_GCC_PRAGMA(str) ML99_PRIV_GCC_PRAGMA(str)

/**
 * The same as #ML99_GCC_PRAGMA but for Clang.
 */
#define ML99_CLANG_PRAGMA(str) ML99_PRIV_CLANG_PRAGMA(str)

#define ML99_CAT(a, b)        ML99_CAT_PRIMITIVE(a, b)
#define ML99_CAT3(a, b, c)    ML99_CAT3_PRIMITIVE(a, b, c)
#define ML99_CAT4(a, b, c, d) ML99_CAT4_PRIMITIVE(a, b, c, d)
#define ML99_STRINGIFY(...)   ML99_STRINGIFY_PRIMITIVE(__VA_ARGS__)
#define ML99_EMPTY(...)
#define ML99_ID(...) __VA_ARGS__

#ifndef DOXYGEN_IGNORE

#define ML99_catEval_IMPL(a, b)      a##b
#define ML99_cat_IMPL(a, b)          v(a##b)
#define ML99_cat3_IMPL(a, b, c)      v(a##b##c)
#define ML99_cat4_IMPL(a, b, c, d)   v(a##b##c##d)
#define ML99_stringify_IMPL(...)     v(ML99_STRINGIFY(__VA_ARGS__))
#define ML99_empty_IMPL(...)         v(ML99_EMPTY())
#define ML99_id_IMPL(...)            v(ML99_ID(__VA_ARGS__))
#define ML99_const_IMPL(x, _a)       v(x)
#define ML99_flip_IMPL(f)            ML99_appl_IMPL(ML99_PRIV_flip, f)
#define ML99_PRIV_flip_IMPL(f, a, b) ML99_appl2_IMPL(f, b, a)
#define ML99_uncomma_IMPL(...)       __VA_ARGS__

#define ML99_reify_IMPL(f)           ML99_appl_IMPL(ML99_PRIV_reify, f)
#define ML99_PRIV_reify_IMPL(f, ...) v(f(__VA_ARGS__))

// clang-format off
#define ML99_todo_IMPL(f) ML99_fatal(f, not yet implemented)
#define ML99_todoWithMsg_IMPL(f, message) ML99_fatal(f, not yet implemented: message)

#define ML99_unimplemented_IMPL(f) ML99_fatal(f, not implemented)
#define ML99_unimplementedWithMsg_IMPL(f, message) ML99_fatal(f, not implemented: message)
// clang-format on

#if defined(__GNUC__) && !defined(__clang__)
#define ML99_PRIV_GCC_PRAGMA(str) _Pragma(str)
#else
#define ML99_PRIV_GCC_PRAGMA(str)
#endif

#if defined(__clang__)
#define ML99_PRIV_CLANG_PRAGMA(str) _Pragma(str)
#else
#define ML99_PRIV_CLANG_PRAGMA(str)
#endif

// Arity specifiers {

#define ML99_catEval_ARITY              2
#define ML99_cat_ARITY                  2
#define ML99_cat3_ARITY                 3
#define ML99_cat4_ARITY                 4
#define ML99_stringify_ARITY            1
#define ML99_empty_ARITY                1
#define ML99_id_ARITY                   1
#define ML99_const_ARITY                2
#define ML99_flip_ARITY                 1
#define ML99_uncomma_ARITY              1
#define ML99_reify_ARITY                1
#define ML99_todo_ARITY                 1
#define ML99_todoWithMsg_ARITY          2
#define ML99_unimplemented_ARITY        1
#define ML99_unimplementedWithMsg_ARITY 2

#define ML99_PRIV_flip_ARITY  3
#define ML99_PRIV_reify_ARITY 2
// } (Arity specifiers)

#endif // DOXYGEN_IGNORE

#endif // ML99_UTIL_H
