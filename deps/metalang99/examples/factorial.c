// `...` is sometimes used to workaround a TCC bug, see
// <https://github.com/Hirrolot/datatype99/issues/10#issuecomment-830813172>.

#include <metalang99.h>

#define factorial(n)          ML99_natMatch(n, v(factorial_))
#define factorial_Z_IMPL(...) v(1) // `...` due to the TCC's bug.
#define factorial_S_IMPL(n)   ML99_mul(ML99_inc(v(n)), factorial(v(n)))

ML99_ASSERT_EQ(factorial(v(0)), v(1));
ML99_ASSERT_EQ(factorial(v(1)), v(1));
ML99_ASSERT_EQ(factorial(v(2)), v(2));
ML99_ASSERT_EQ(factorial(v(3)), v(6));
ML99_ASSERT_EQ(factorial(v(4)), v(24));

int main(void) {}
