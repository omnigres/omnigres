// Asserts multiple expressions at once.

#include <metalang99.h>

#include <assert.h>

#define ASSERT_FOR_EACH(...)                                                                       \
    do {                                                                                           \
        ML99_EVAL(ML99_variadicsForEach(                                                           \
            ML99_compose(v(ML99_semicoloned), ML99_reify(v(assert))),                              \
            v(__VA_ARGS__)))                                                                       \
    } while (0)

int main(void) {
    ASSERT_FOR_EACH(123 == 123, 2 + 2 == 4, "foo"[1] == 'o');

    /*
     * If we combine multiple assertions with the && operator, we will not be able to distinguish
     * them if one of them fails apparently:
     *
     * main: Assertion `123 == 321 && 2 + 2 == 4 && "foo"[1] == 'o' failed.
     * assert(123 == 321 && 2 + 2 == 4 && "foo"[1] == 'o');
     */

    /*
     * ... unlike `ASSERT_FOR_EACH` telling us which one has failed:
     *
     * main: Assertion `123 == 321' failed.
     * ASSERT_FOR_EACH(123 == 321, 2 + 2 == 4, "foo"[1] == 'o');
     */
}
