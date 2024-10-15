#ifndef ML99_PRIV_UTIL_H
#define ML99_PRIV_UTIL_H

#define ML99_PRIV_CAT(x, y)           ML99_PRIV_PRIMITIVE_CAT(x, y)
#define ML99_PRIV_PRIMITIVE_CAT(x, y) x##y

#define ML99_PRIV_CAT3(x, y, z)           ML99_PRIV_PRIMITIVE_CAT3(x, y, z)
#define ML99_PRIV_PRIMITIVE_CAT3(x, y, z) x##y##z

#define ML99_PRIV_EXPAND(...) __VA_ARGS__
#define ML99_PRIV_EMPTY(...)
#define ML99_PRIV_COMMA(...) ,

#define ML99_PRIV_HEAD(...)        ML99_PRIV_HEAD_AUX(__VA_ARGS__, ~)
#define ML99_PRIV_HEAD_AUX(x, ...) x

#define ML99_PRIV_TAIL(...)         ML99_PRIV_TAIL_AUX(__VA_ARGS__)
#define ML99_PRIV_TAIL_AUX(_x, ...) __VA_ARGS__

#define ML99_PRIV_SND(...)            ML99_PRIV_SND_AUX(__VA_ARGS__, ~)
#define ML99_PRIV_SND_AUX(_x, y, ...) y

#define ML99_PRIV_CONTAINS_COMMA(...)                      ML99_PRIV_X_AS_COMMA(__VA_ARGS__, ML99_PRIV_COMMA(), ~)
#define ML99_PRIV_X_AS_COMMA(_head, x, ...)                ML99_PRIV_CONTAINS_COMMA_RESULT(x, 0, 1, ~)
#define ML99_PRIV_CONTAINS_COMMA_RESULT(x, _, result, ...) result

#endif // ML99_PRIV_UTIL_H
