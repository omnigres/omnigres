/**
 * @file
 * Choice types: `(tag, ...)`.
 *
 * A choice type, also known as [tagged union], is represented as `(tag, ...)`, where `tag` is the
 * type of a value and `...` is the value. Perhaps the most common example of a choice type is a
 * binary tree:
 *
 * [[examples/binary_tree.c](https://github.com/Hirrolot/metalang99/blob/master/examples/binary_tree.c)]
 * @include binary_tree.c
 *
 * [tagged union]: https://en.wikipedia.org/wiki/Tagged_union
 */

#ifndef ML99_CHOICE_H
#define ML99_CHOICE_H

#include <metalang99/priv/util.h>

#include <metalang99/lang.h>

/**
 * Constructs an instance of a choice type.
 *
 * # Examples
 *
 * See
 * [examples/binary_tree.c](https://github.com/Hirrolot/metalang99/blob/master/examples/binary_tree.c).
 *
 * @note Specify `~` if you do not want to supply data; then, to match it, write a `_` parameter to
 * ignore.
 */
#define ML99_choice(tag, ...) ML99_call(ML99_choice, tag, __VA_ARGS__)

/**
 * Evaluates to the tag of @p choice.
 *
 * This macro is essentially the same as `ML99_tupleGet(0)`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/choice.h>
 *
 * // foo
 * ML99_choiceTag(ML99_choice(v(foo), v(1, 2, 3)))
 * @endcode
 */
#define ML99_choiceTag(choice) ML99_call(ML99_choiceTag, choice)

/**
 * Evaluates to the data of @p choice.
 *
 * This macro is essentially the same as #ML99_tupleTail.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/choice.h>
 *
 * // 1, 2, 3
 * ML99_choiceData(ML99_choice(v(foo), v(1, 2, 3)))
 * @endcode
 */
#define ML99_choiceData(choice) ML99_call(ML99_choiceData, choice)

/**
 * Matches the instance @p choice of a choice type.
 *
 * This macro results in `ML99_call(ML99_cat(matcher, ML99_choiceTag(choice)), <choice data>)`.
 *
 * # Examples
 *
 * See
 * [examples/binary_tree.c](https://github.com/Hirrolot/metalang99/blob/master/examples/binary_tree.c).
 */
#define ML99_match(choice, matcher) ML99_call(ML99_match, choice, matcher)

/**
 * The same as #ML99_match but supplies additional arguments to all branches.
 *
 * This macro results in `ML99_call(ML99_cat(matcher, ML99_choiceTag(choice)), <choice data>,
 * args...)`.
 *
 * # Examples
 *
 * @code
 * #include <metalang99/choice.h>
 *
 * #define MATCH_A_IMPL(x, y, z) v(x ~ y ~ z)
 *
 * // 123 ~ 456 ~ 789
 * ML99_matchWithArgs(ML99_choice(v(A), v(123)), v(MATCH_), v(456, 789))
 * @endcode
 */
#define ML99_matchWithArgs(choice, matcher, ...)                                                   \
    ML99_call(ML99_matchWithArgs, choice, matcher, __VA_ARGS__)

#define ML99_CHOICE(tag, ...)    (tag, __VA_ARGS__)
#define ML99_CHOICE_TAG(choice)  ML99_PRIV_HEAD_AUX choice
#define ML99_CHOICE_DATA(choice) ML99_PRIV_TAIL_AUX choice

#ifndef DOXYGEN_IGNORE

#define ML99_choice_IMPL(tag, ...)   v(ML99_CHOICE(tag, __VA_ARGS__))
#define ML99_choiceTag_IMPL(choice)  v(ML99_CHOICE_TAG(choice))
#define ML99_choiceData_IMPL(choice) v(ML99_CHOICE_DATA(choice))

#define ML99_match_IMPL(choice, matcher)                                                           \
    ML99_callUneval(ML99_PRIV_CAT(matcher, ML99_PRIV_HEAD_AUX choice), ML99_PRIV_TAIL_AUX choice)

#define ML99_matchWithArgs_IMPL(choice, matcher, ...)                                              \
    ML99_callUneval(                                                                               \
        ML99_PRIV_CAT(matcher, ML99_PRIV_HEAD_AUX choice),                                         \
        ML99_PRIV_TAIL_AUX choice,                                                                 \
        __VA_ARGS__)

// Arity specifiers {

#define ML99_choice_ARITY        2
#define ML99_choiceTag_ARITY     1
#define ML99_choiceData_ARITY    1
#define ML99_match_ARITY         2
#define ML99_matchWithArgs_ARITY 3
// } (Arity specifiers)

#endif // DOXYGEN_IGNORE

#endif // ML99_CHOICE_H
