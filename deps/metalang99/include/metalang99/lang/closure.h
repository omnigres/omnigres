#ifndef ML99_LANG_CLOSURE_H
#define ML99_LANG_CLOSURE_H

#include <metalang99/priv/bool.h>
#include <metalang99/priv/util.h>

#include <metalang99/nat/dec.h>
#include <metalang99/nat/eq.h>

/*
 * A closure has the form `(arity, f, ...)`, where `arity` is how many times `ML99_appl` can
 * be called for this closure, and `...` denotes the closure's environment.
 *
 * `METALANG_appl` is described by the following algorithm:
 *  - If `f` is an identifier (like `FOO`):
 *    - If `f##_ARITY` is 1, then just call this function with provided arguments.
 *    - Otherwise, return `(f##_ARITY - 1, f, provided args...)`.
 *  - Otherwise (`f` is a closure):
 *    - If `arity` is 1, then just call `f` with its environment and provided arguments.
 *    - Otherwise, return `(arity - 1, f, env..., provided args...)`.
 *
 * Thus, each time except the last, `ML99_appl` extends a closure's environment with new
 * arguments; the last time, it calls `f` with its environment.
 */

#define ML99_appl_IMPL(f, ...)                                                                     \
    ML99_PRIV_IF(ML99_PRIV_IS_UNTUPLE_FAST(f), ML99_PRIV_APPL_F, ML99_PRIV_APPL_CLOSURE)           \
    (f, __VA_ARGS__)

#define ML99_PRIV_APPL_F(f, ...)                                                                   \
    ML99_PRIV_IF(                                                                                  \
        ML99_PRIV_NAT_EQ(f##_ARITY, 1),                                                            \
        ML99_callUneval(f, __VA_ARGS__),                                                           \
        v((ML99_PRIV_DEC(f##_ARITY), f, __VA_ARGS__)))

#define ML99_PRIV_APPL_CLOSURE(closure, ...)                                                       \
    ML99_PRIV_APPL_CLOSURE_AUX(ML99_PRIV_EXPAND closure, __VA_ARGS__)

#define ML99_PRIV_APPL_CLOSURE_AUX(...) ML99_PRIV_APPL_CLOSURE_AUX_AUX(__VA_ARGS__)

#define ML99_PRIV_APPL_CLOSURE_AUX_AUX(arity, f, ...)                                              \
    ML99_PRIV_IF(                                                                                  \
        ML99_PRIV_NAT_EQ(arity, 1),                                                                \
        ML99_callUneval(f, __VA_ARGS__),                                                           \
        v((ML99_PRIV_DEC(arity), f, __VA_ARGS__)))

#define ML99_appl2_IMPL(f, a, b)       ML99_appl(ML99_appl_IMPL(f, a), v(b))
#define ML99_appl3_IMPL(f, a, b, c)    ML99_appl(ML99_appl2_IMPL(f, a, b), v(c))
#define ML99_appl4_IMPL(f, a, b, c, d) ML99_appl(ML99_appl3_IMPL(f, a, b, c), v(d))

#endif // ML99_LANG_CLOSURE_H
