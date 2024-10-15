// `...` is sometimes used to workaround a TCC bug, see
// <https://github.com/Hirrolot/datatype99/issues/10#issuecomment-830813172>.

#include <metalang99.h>

// Compile-time list manipulation:

// 3, 3, 3, 3, 3
static int five_threes[] = {
    ML99_LIST_EVAL_COMMA_SEP(ML99_listReplicate(v(5), v(3))),
};

// 5, 4, 3, 2, 1
static int from_5_to_1[] = {
    ML99_LIST_EVAL_COMMA_SEP(ML99_listReverse(ML99_list(v(1, 2, 3, 4, 5)))),
};

// 9, 2, 5
static int lesser_than_10[] = {
    ML99_LIST_EVAL_COMMA_SEP(
        ML99_listFilter(ML99_appl(v(ML99_greater), v(10)), ML99_list(v(9, 2, 11, 13, 5)))),
};

// Macro recursion:
#define factorial(n)          ML99_natMatch(n, v(factorial_))
#define factorial_Z_IMPL(...) v(1) // `...` due to the TCC's bug.
#define factorial_S_IMPL(n)   ML99_mul(ML99_inc(v(n)), factorial(v(n)))

ML99_ASSERT_EQ(factorial(v(4)), v(24));

// Overloading on a number of arguments:
typedef struct {
    double width, height;
} Rect;

#define Rect_new(...) ML99_OVERLOAD(Rect_new_, __VA_ARGS__)
#define Rect_new_1(x)                                                                              \
    { x, x }
#define Rect_new_2(x, y)                                                                           \
    { x, y }

static Rect _7x8 = Rect_new(7, 8), _10x10 = Rect_new(10);

// ... and more!

int main(void) {
    // Yeah. All is done at compile time.
}
