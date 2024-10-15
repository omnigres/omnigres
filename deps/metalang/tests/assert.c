#include <metalang99/assert.h>

// This is used to check that `1 == 1` is put into parentheses automatically.
#define COND 1 == 1

ML99_EVAL(ML99_assert(v(COND)));
ML99_EVAL(ML99_assertEq(v(COND), v(COND)));

ML99_ASSERT(v(COND));
ML99_ASSERT_EQ(v(COND), v(COND));

ML99_ASSERT_UNEVAL(COND);

#undef COND

ML99_ASSERT_EMPTY(v());
ML99_ASSERT_EMPTY_UNEVAL();

int main(void) {}
