#ifndef ML99_PRIV_TUPLE_H
#define ML99_PRIV_TUPLE_H

#include <metalang99/priv/bool.h>
#include <metalang99/priv/util.h>

#define ML99_PRIV_IS_TUPLE(x)      ML99_PRIV_NOT(ML99_PRIV_IS_UNTUPLE(x))
#define ML99_PRIV_IS_TUPLE_FAST(x) ML99_PRIV_NOT(ML99_PRIV_IS_UNTUPLE_FAST(x))

#define ML99_PRIV_IS_UNTUPLE(x)                                                                    \
    ML99_PRIV_IF(                                                                                  \
        ML99_PRIV_IS_DOUBLE_TUPLE_BEGINNING(x),                                                    \
        ML99_PRIV_TRUE,                                                                            \
        ML99_PRIV_IS_UNTUPLE_FAST)                                                                 \
    (x)

#define ML99_PRIV_IS_UNTUPLE_FAST(x)        ML99_PRIV_SND(ML99_PRIV_IS_UNTUPLE_FAST_TEST x, 1)
#define ML99_PRIV_IS_UNTUPLE_FAST_TEST(...) ~, 0

#define ML99_PRIV_UNTUPLE(x) ML99_PRIV_EXPAND x

/**
 * Checks whether @p x takes the form `(...) (...) ...`.
 *
 * This often happens when you miss a comma between items:
 *  - `v(123) v(456)`
 *  - `(Foo, int) (Bar, int)` (as in Datatype99)
 *  - etc.
 */
#define ML99_PRIV_IS_DOUBLE_TUPLE_BEGINNING(x)                                                     \
    ML99_PRIV_CONTAINS_COMMA(ML99_PRIV_EXPAND(ML99_PRIV_COMMA ML99_PRIV_EMPTY x))

#endif // ML99_PRIV_TUPLE_H
