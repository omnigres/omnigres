/**
 * @file
 * Sequences: `(x)(y)(z)`.
 *
 * A sequence is represented as `(...) (...) ...`. For example, these are sequences:
 *  - `(~, ~, ~)`
 *  - `(1)(2)(3)`
 *  - `(+, -, *, /)(123)(~)`
 *
 * Sequences can represent syntax like `X(...) Y(...) Z(...)`, where `X`, `Y`, and `Z` expand to a
 * [tuple](tuple.html), thereby forming a sequence. A perfect example is
 * [Interface99](https://github.com/Hirrolot/interface99), which allows a user to define a software
 * interface via a number of `vfunc(...)` macro invocations:
 *
 * @code
 * #define Shape_IFACE                      \
 *     vfunc( int, perim, const VSelf)      \
 *     vfunc(void, scale, VSelf, int factor)
 *
 * interface(Shape);
 * @endcode
 *
 * With `vfunc` being defined as follows (simplified):
 *
 * @code
 * #define vfunc(ret_ty, name, ...) (ret_ty, name, __VA_ARGS__)
 * @endcode
 *
 * @note Sequences are more time and space-efficient than lists, but export less functionality; if a
 * needed function is missed, invoking #ML99_listFromSeq and then manipulating with the resulting
 * Cons-list might be helpful.
 */

#ifndef ML99_SEQ_H
#define ML99_SEQ_H

#include <metalang99/nat/inc.h>
#include <metalang99/priv/tuple.h>
#include <metalang99/priv/util.h>

#include <metalang99/lang.h>

/**
 * True iff @p seq contains no elements (which means an empty preprocessing lexeme).
 *
 * # Examples
 *
 * @code
 * #include <metalang99/seq.h>
 *
 * // 1
 * ML99_seqIsEmpty(v())
 *
 * // 0
 * ML99_seqIsEmpty(v((~)(~)(~)))
 * @endcode
 */
#define ML99_seqIsEmpty(seq) ML99_call(ML99_seqIsEmpty, seq)

/**
 * Expands to a metafunction extracting the @p i -indexed element of @p seq.
 *
 * @p i can range from 0 to 7, inclusively.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/seq.h>
 *
 * // 2
 * ML99_seqGet(1)(v((1)(2)(3)))
 * @endcode
 */
#define ML99_seqGet(i) ML99_PRIV_CAT(ML99_PRIV_seqGet_, i)

/**
 * Extracts the tail of @p seq.
 *
 * @p seq must contain at least one element. If @p seq contains **only** one element, the result is
 * `ML99_empty()`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/seq.h>
 *
 * // (2)(3)
 * ML99_seqTail(v((1)(2)(3)))
 * @endcode
 */
#define ML99_seqTail(seq) ML99_call(ML99_seqTail, seq)

/**
 * Applies @p f to each element in @p seq.
 *
 * The result is `ML99_appl(f, x1) ... ML99_appl(f, xN)`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/seq.h>
 *
 * #define F_IMPL(x) v(@x)
 * #define F_ARITY   1
 *
 * // @x @y @z
 * ML99_seqForEach(v(F), v((x)(y)(z)))
 * @endcode
 */
#define ML99_seqForEach(f, seq) ML99_call(ML99_seqForEach, f, seq)

/**
 * Applies @p f to each element in @p seq with an index.
 *
 * The result is `ML99_appl2(f, 0, x1) ... ML99_appl2(f, N - 1, xN)`.
 *
 * @code
 * #include <metalang99/seq.h>
 *
 * #define F_IMPL(i, x) v(@x##i)
 * #define F_ARITY      2
 *
 * // @x0 @y1 @z2
 * ML99_seqForEachI(v(F), v((x)(y)(z)))
 * @endcode
 */
#define ML99_seqForEachI(f, seq) ML99_call(ML99_seqForEachI, f, seq)

#define ML99_SEQ_IS_EMPTY(seq) ML99_PRIV_NOT(ML99_PRIV_CONTAINS_COMMA(ML99_PRIV_COMMA seq))
#define ML99_SEQ_GET(i)        ML99_PRIV_CAT(ML99_PRIV_SEQ_GET_, i)
#define ML99_SEQ_TAIL(seq)     ML99_PRIV_TAIL(ML99_PRIV_COMMA seq)

#ifndef DOXYGEN_IGNORE

#define ML99_seqIsEmpty_IMPL(seq) v(ML99_SEQ_IS_EMPTY(seq))

#define ML99_PRIV_seqGet_0(seq) ML99_call(ML99_PRIV_seqGet_0, seq)
#define ML99_PRIV_seqGet_1(seq) ML99_call(ML99_PRIV_seqGet_1, seq)
#define ML99_PRIV_seqGet_2(seq) ML99_call(ML99_PRIV_seqGet_2, seq)
#define ML99_PRIV_seqGet_3(seq) ML99_call(ML99_PRIV_seqGet_3, seq)
#define ML99_PRIV_seqGet_4(seq) ML99_call(ML99_PRIV_seqGet_4, seq)
#define ML99_PRIV_seqGet_5(seq) ML99_call(ML99_PRIV_seqGet_5, seq)
#define ML99_PRIV_seqGet_6(seq) ML99_call(ML99_PRIV_seqGet_6, seq)
#define ML99_PRIV_seqGet_7(seq) ML99_call(ML99_PRIV_seqGet_7, seq)

#define ML99_PRIV_seqGet_0_IMPL(seq) v(ML99_SEQ_GET(0)(seq))
#define ML99_PRIV_seqGet_1_IMPL(seq) v(ML99_SEQ_GET(1)(seq))
#define ML99_PRIV_seqGet_2_IMPL(seq) v(ML99_SEQ_GET(2)(seq))
#define ML99_PRIV_seqGet_3_IMPL(seq) v(ML99_SEQ_GET(3)(seq))
#define ML99_PRIV_seqGet_4_IMPL(seq) v(ML99_SEQ_GET(4)(seq))
#define ML99_PRIV_seqGet_5_IMPL(seq) v(ML99_SEQ_GET(5)(seq))
#define ML99_PRIV_seqGet_6_IMPL(seq) v(ML99_SEQ_GET(6)(seq))
#define ML99_PRIV_seqGet_7_IMPL(seq) v(ML99_SEQ_GET(7)(seq))

#define ML99_PRIV_SEQ_GET_0(seq) ML99_PRIV_UNTUPLE(ML99_PRIV_HEAD(ML99_PRIV_SEQ_SEPARATE seq))
#define ML99_PRIV_SEQ_GET_1(seq) ML99_PRIV_SEQ_GET_0(ML99_SEQ_TAIL(seq))
#define ML99_PRIV_SEQ_GET_2(seq) ML99_PRIV_SEQ_GET_1(ML99_SEQ_TAIL(seq))
#define ML99_PRIV_SEQ_GET_3(seq) ML99_PRIV_SEQ_GET_2(ML99_SEQ_TAIL(seq))
#define ML99_PRIV_SEQ_GET_4(seq) ML99_PRIV_SEQ_GET_3(ML99_SEQ_TAIL(seq))
#define ML99_PRIV_SEQ_GET_5(seq) ML99_PRIV_SEQ_GET_4(ML99_SEQ_TAIL(seq))
#define ML99_PRIV_SEQ_GET_6(seq) ML99_PRIV_SEQ_GET_5(ML99_SEQ_TAIL(seq))
#define ML99_PRIV_SEQ_GET_7(seq) ML99_PRIV_SEQ_GET_6(ML99_SEQ_TAIL(seq))

#define ML99_PRIV_SEQ_SEPARATE(...) (__VA_ARGS__),

#define ML99_seqTail_IMPL(seq) v(ML99_SEQ_TAIL(seq))

#define ML99_seqForEach_IMPL(f, seq)                                                               \
    ML99_PRIV_CAT(ML99_PRIV_seqForEach_, ML99_SEQ_IS_EMPTY(seq))(f, seq)
#define ML99_PRIV_seqForEach_1(...) v(ML99_PRIV_EMPTY())
#define ML99_PRIV_seqForEach_0(f, seq)                                                             \
    ML99_TERMS(                                                                                    \
        ML99_appl_IMPL(f, ML99_SEQ_GET(0)(seq)),                                                   \
        ML99_callUneval(ML99_seqForEach, f, ML99_SEQ_TAIL(seq)))

#define ML99_seqForEachI_IMPL(f, seq) ML99_PRIV_seqForEachIAux_IMPL(f, 0, seq)
#define ML99_PRIV_seqForEachIAux_IMPL(f, i, seq)                                                   \
    ML99_PRIV_CAT(ML99_PRIV_seqForEachI_, ML99_SEQ_IS_EMPTY(seq))(f, i, seq)
#define ML99_PRIV_seqForEachI_1(...) v(ML99_PRIV_EMPTY())
#define ML99_PRIV_seqForEachI_0(f, i, seq)                                                         \
    ML99_TERMS(                                                                                    \
        ML99_appl2_IMPL(f, i, ML99_SEQ_GET(0)(seq)),                                               \
        ML99_callUneval(ML99_PRIV_seqForEachIAux, f, ML99_PRIV_INC(i), ML99_SEQ_TAIL(seq)))

// Arity specifiers {

#define ML99_seqIsEmpty_ARITY  1
#define ML99_seqTail_ARITY     1
#define ML99_seqForEach_ARITY  2
#define ML99_seqForEachI_ARITY 2

#define ML99_PRIV_seqGet_0_ARITY 1
#define ML99_PRIV_seqGet_1_ARITY 1
#define ML99_PRIV_seqGet_2_ARITY 1
#define ML99_PRIV_seqGet_3_ARITY 1
#define ML99_PRIV_seqGet_4_ARITY 1
#define ML99_PRIV_seqGet_5_ARITY 1
#define ML99_PRIV_seqGet_6_ARITY 1
#define ML99_PRIV_seqGet_7_ARITY 1
// } (Arity specifiers)

#endif // DOXYGEN_IGNORE

#endif // ML99_SEQ_H
