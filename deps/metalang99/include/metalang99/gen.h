/**
 * @file
 * Support for C language constructions.
 *
 * Some decent usage examples can be found in
 * [datatype99/examples/derive](https://github.com/Hirrolot/datatype99/tree/master/examples/derive).
 */

#ifndef ML99_GEN_H
#define ML99_GEN_H

#include <metalang99/priv/bool.h>

#include <metalang99/choice.h>
#include <metalang99/lang.h>
#include <metalang99/list.h>
#include <metalang99/nat.h>
#include <metalang99/tuple.h>
#include <metalang99/variadics.h>

#include <metalang99/stmt.h> // For backwards compatibility.
#include <metalang99/util.h> // For backwards compatibility: ML99_GEN_SYM, ML99_TRAILING_SEMICOLON.

/**
 * Puts a semicolon after provided arguments.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // int x = 5;
 * ML99_semicoloned(v(int x = 5))
 * @endcode
 */
#define ML99_semicoloned(...) ML99_call(ML99_semicoloned, __VA_ARGS__)

/**
 * Puts provided arguments into braces.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // { int a, b, c; }
 * ML99_braced(v(int a, b, c;))
 * @endcode
 */
#define ML99_braced(...) ML99_call(ML99_braced, __VA_ARGS__)

/**
 * Generates an assignment of provided variadic arguments to @p lhs.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // x = 5, 6, 7
 * ML99_assign(v(x), v(5, 6, 7))
 * @endcode
 */
#define ML99_assign(lhs, ...) ML99_call(ML99_assign, lhs, __VA_ARGS__)

/**
 * A shortcut for `ML99_assign(lhs, ML99_braced(...))`.
 */
#define ML99_assignInitializerList(lhs, ...) ML99_call(ML99_assignInitializerList, lhs, __VA_ARGS__)

/**
 * A shortcut for `ML99_semicoloned(ML99_assign(lhs, ...))`.
 */
#define ML99_assignStmt(lhs, ...) ML99_call(ML99_assignStmt, lhs, __VA_ARGS__)

/**
 * A shortcut for `ML99_assignStmt(lhs, ML99_braced(...))`.
 */
#define ML99_assignInitializerListStmt(lhs, ...)                                                   \
    ML99_call(ML99_assignInitializerListStmt, lhs, __VA_ARGS__)

/**
 * Generates a function/macro invocation.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // If you are on C11.
 * ML99_invoke(v(_Static_assert), v(1 == 1, "Must be true"))
 * @endcode
 */
#define ML99_invoke(f, ...) ML99_call(ML99_invoke, f, __VA_ARGS__)

/**
 * A shortcut for `ML99_semicoloned(ML99_invoke(f, ...))`.
 */
#define ML99_invokeStmt(f, ...) ML99_call(ML99_invokeStmt, f, __VA_ARGS__)

/**
 * Generates `prefix { code }`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // if (1 == 1) {
 * //     printf("x = %d\n", x);
 * // }
 * ML99_prefixedBlock(v(if (1 == 1)), v(printf("x = %d\n", x);))
 * @endcode
 */
#define ML99_prefixedBlock(prefix, ...) ML99_call(ML99_prefixedBlock, prefix, __VA_ARGS__)

/**
 * Generates a type definition.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // typedef struct { int x, y; } Point;
 * ML99_typedef(v(Point), v(struct { int x, y; }))
 * @endcode
 */
#define ML99_typedef(ident, ...) ML99_call(ML99_typedef, ident, __VA_ARGS__)

/**
 * Generates a C structure.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // struct Point { int x, y; }
 * ML99_struct(v(Point), v(int x, y;))
 * @endcode
 */
#define ML99_struct(ident, ...) ML99_call(ML99_struct, ident, __VA_ARGS__)

/**
 * Generates an anonymous C structure.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // struct { int x, y; }
 * ML99_struct(v(int x, y;))
 * @endcode
 */
#define ML99_anonStruct(...) ML99_call(ML99_anonStruct, __VA_ARGS__)

/**
 * The same as #ML99_struct but generates a union.
 */
#define ML99_union(ident, ...) ML99_call(ML99_union, ident, __VA_ARGS__)

/**
 * The same as #ML99_anonStruct but generates a union.
 */
#define ML99_anonUnion(...) ML99_call(ML99_anonUnion, __VA_ARGS__)

/**
 * The same as #ML99_struct but generates an enumeration.
 */
#define ML99_enum(ident, ...) ML99_call(ML99_enum, ident, __VA_ARGS__)

/**
 * The same as #ML99_anonStruct but generates an enumeration.
 */
#define ML99_anonEnum(...) ML99_call(ML99_anonEnum, __VA_ARGS__)

/**
 * Generates a function pointer.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // int (*add)(int x, int y)
 * ML99_fnPtr(v(int), v(add), v(int x), v(int y))
 *
 * // const char *(*title)(void)
 * ML99_fnPtr(v(const char *), v(title), v(void))
 * @endcode
 */
#define ML99_fnPtr(ret_ty, name, ...) ML99_call(ML99_fnPtr, ret_ty, name, __VA_ARGS__)

/**
 * A shortcut for `ML99_semicoloned(ML99_fnPtr(ret_ty, name, ...))`.
 */
#define ML99_fnPtrStmt(ret_ty, name, ...) ML99_call(ML99_fnPtrStmt, ret_ty, name, __VA_ARGS__)

/**
 * Pastes provided arguments @p n times.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // ~ ~ ~ ~ ~
 * ML99_times(v(5), v(~))
 * @endcode
 */
#define ML99_times(n, ...) ML99_call(ML99_times, n, __VA_ARGS__)

/**
 * Invokes @p f @p n times, providing an iteration index each time.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 * #include <metalang99/util.h>
 *
 * // _0 _1 _2
 * ML99_repeat(v(3), ML99_appl(v(ML99_cat), v(_)))
 * @endcode
 */
#define ML99_repeat(n, f) ML99_call(ML99_repeat, n, f)

/**
 * Generates \f$(T_0 \ \_0, ..., T_n \ \_n)\f$.
 *
 * If @p type_list is empty, this macro results in `(void)`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // (int _0, long long _1, const char * _2)
 * ML99_indexedParams(ML99_list(v(int, long long, const char *)))
 *
 * // (void)
 * ML99_indexedParams(ML99_nil())
 * @endcode
 */
#define ML99_indexedParams(type_list) ML99_call(ML99_indexedParams, type_list)

/**
 * Generates \f$T_0 \ \_0; ...; T_n \ \_n\f$.
 *
 * If @p type_list is empty, this macro results in emptiness.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // int _0; long long _1; const char * _2;
 * ML99_indexedFields(ML99_list(v(int, long long, const char *)))
 *
 * // ML99_empty()
 * ML99_indexedFields(ML99_nil())
 * @endcode
 */
#define ML99_indexedFields(type_list) ML99_call(ML99_indexedFields, type_list)

/**
 * Generates \f$\{ \_0, ..., \_{n - 1} \}\f$.
 *
 * If @p n is 0, this macro results in `{ 0 }`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // { _0, _1, _2 }
 * ML99_indexedInitializerList(v(3))
 *
 * // { 0 }
 * ML99_indexedInitializerList(v(0))
 * @endcode
 */
#define ML99_indexedInitializerList(n) ML99_call(ML99_indexedInitializerList, n)

/**
 * Generates \f$\_0, ..., \_{n - 1}\f$.
 *
 * If @p n is 0, this macro results in emptiness.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/gen.h>
 *
 * // _0, _1, _2
 * ML99_indexedArgs(v(3))
 *
 * // ML99_empty()
 * ML99_indexedArgs(v(0))
 * @endcode
 */
#define ML99_indexedArgs(n) ML99_call(ML99_indexedArgs, n)

#ifndef DOXYGEN_IGNORE

#define ML99_semicoloned_IMPL(...)                    v(__VA_ARGS__;)
#define ML99_braced_IMPL(...)                         v({__VA_ARGS__})
#define ML99_assign_IMPL(lhs, ...)                    v(lhs = __VA_ARGS__)
#define ML99_assignStmt_IMPL(lhs, ...)                v(lhs = __VA_ARGS__;)
#define ML99_assignInitializerList_IMPL(lhs, ...)     v(lhs = {__VA_ARGS__})
#define ML99_assignInitializerListStmt_IMPL(lhs, ...) v(lhs = {__VA_ARGS__};)
#define ML99_invoke_IMPL(f, ...)                      v(f(__VA_ARGS__))
#define ML99_invokeStmt_IMPL(f, ...)                  v(f(__VA_ARGS__);)
#define ML99_typedef_IMPL(ident, ...)                 v(typedef __VA_ARGS__ ident;)
#define ML99_fnPtr_IMPL(ret_ty, name, ...)            v(ret_ty (*name)(__VA_ARGS__))
#define ML99_fnPtrStmt_IMPL(ret_ty, name, ...)        v(ret_ty (*name)(__VA_ARGS__);)

// clang-format off
#define ML99_prefixedBlock_IMPL(prefix, ...) v(prefix {__VA_ARGS__})
#define ML99_struct_IMPL(ident, ...) v(struct ident {__VA_ARGS__})
#define ML99_anonStruct_IMPL(...) v(struct {__VA_ARGS__})
#define ML99_union_IMPL(ident, ...) v(union ident {__VA_ARGS__})
#define ML99_anonUnion_IMPL(...) v(union {__VA_ARGS__})
#define ML99_enum_IMPL(ident, ...) v(enum ident {__VA_ARGS__})
#define ML99_anonEnum_IMPL(...) v(enum {__VA_ARGS__})
// clang-format on

#define ML99_times_IMPL(n, ...)        ML99_natMatchWithArgs_IMPL(n, ML99_PRIV_times_, __VA_ARGS__)
#define ML99_PRIV_times_Z_IMPL         ML99_empty_IMPL
#define ML99_PRIV_times_S_IMPL(i, ...) ML99_TERMS(v(__VA_ARGS__), ML99_times_IMPL(i, __VA_ARGS__))

#define ML99_repeat_IMPL(n, f)        ML99_natMatchWithArgs_IMPL(n, ML99_PRIV_repeat_, f)
#define ML99_PRIV_repeat_Z_IMPL       ML99_empty_IMPL
#define ML99_PRIV_repeat_S_IMPL(i, f) ML99_TERMS(ML99_repeat_IMPL(i, f), ML99_appl_IMPL(f, i))

// ML99_indexedParams_IMPL {

#define ML99_indexedParams_IMPL(type_list)                                                         \
    ML99_tuple(ML99_PRIV_IF(                                                                       \
        ML99_IS_NIL(type_list),                                                                    \
        v(void),                                                                                   \
        ML99_variadicsTail(ML99_PRIV_indexedParamsAux_IMPL(type_list, 0))))

#define ML99_PRIV_indexedParamsAux_IMPL(type_list, i)                                              \
    ML99_matchWithArgs_IMPL(type_list, ML99_PRIV_indexedParams_, i)
#define ML99_PRIV_indexedParams_nil_IMPL ML99_empty_IMPL
#define ML99_PRIV_indexedParams_cons_IMPL(x, xs, i)                                                \
    ML99_TERMS(v(, x _##i), ML99_PRIV_indexedParamsAux_IMPL(xs, ML99_INC(i)))
// } (ML99_indexedParams_IMPL)

// ML99_indexedFields_IMPL {

#define ML99_indexedFields_IMPL(type_list) ML99_PRIV_indexedFieldsAux_IMPL(type_list, 0)

#define ML99_PRIV_indexedFieldsAux_IMPL(type_list, i)                                              \
    ML99_matchWithArgs_IMPL(type_list, ML99_PRIV_indexedFields_, i)
#define ML99_PRIV_indexedFields_nil_IMPL ML99_empty_IMPL
#define ML99_PRIV_indexedFields_cons_IMPL(x, xs, i)                                                \
    ML99_TERMS(v(x _##i;), ML99_PRIV_indexedFieldsAux_IMPL(xs, ML99_INC(i)))
// } (ML99_indexedFields_IMPL)

#define ML99_indexedInitializerList_IMPL(n) ML99_braced(ML99_PRIV_INDEXED_ITEMS(n, v(0)))
#define ML99_indexedArgs_IMPL(n)            ML99_PRIV_INDEXED_ITEMS(n, v(ML99_EMPTY()))

#define ML99_PRIV_INDEXED_ITEMS(n, empty_case)                                                     \
    ML99_PRIV_IF(                                                                                  \
        ML99_NAT_EQ(n, 0),                                                                         \
        empty_case,                                                                                \
        ML99_variadicsTail(ML99_repeat_IMPL(n, ML99_PRIV_indexedItem)))

#define ML99_PRIV_indexedItem_IMPL(i) v(, _##i)

// Arity specifiers {

#define ML99_semicoloned_ARITY               1
#define ML99_braced_ARITY                    1
#define ML99_assign_ARITY                    2
#define ML99_assignStmt_ARITY                2
#define ML99_assignInitializerList_ARITY     2
#define ML99_assignInitializerListStmt_ARITY 2
#define ML99_invoke_ARITY                    2
#define ML99_invokeStmt_ARITY                2
#define ML99_prefixedBlock_ARITY             2
#define ML99_typedef_ARITY                   2
#define ML99_struct_ARITY                    2
#define ML99_anonStruct_ARITY                1
#define ML99_union_ARITY                     2
#define ML99_anonUnion_ARITY                 1
#define ML99_enum_ARITY                      2
#define ML99_anonEnum_ARITY                  1
#define ML99_fnPtr_ARITY                     3
#define ML99_fnPtrStmt_ARITY                 3
#define ML99_repeat_ARITY                    2
#define ML99_times_ARITY                     2
#define ML99_indexedParams_ARITY             1
#define ML99_indexedFields_ARITY             1
#define ML99_indexedInitializerList_ARITY    1
#define ML99_indexedArgs_ARITY               1

#define ML99_PRIV_indexedItem_ARITY 1
// } (Arity specifiers)

#endif // DOXYGEN_IGNORE

#endif // ML99_GEN_H
