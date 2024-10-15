#include <metalang99.h>

#define ack(m, n) ML99_natMatchWithArgs(m, v(ack_), n)

#define ack_Z_IMPL(n)      ML99_inc(v(n))
#define ack_S_IMPL(m, n)   ML99_natMatchWithArgs(v(n), v(ack_S_), v(m))
#define ack_S_Z_IMPL(m)    ack(v(m), v(1))
#define ack_S_S_IMPL(n, m) ack(v(m), ack(ML99_inc(v(m)), v(n)))

ML99_ASSERT_EQ(ack(v(0), v(0)), v(1));
ML99_ASSERT_EQ(ack(v(0), v(1)), v(2));
ML99_ASSERT_EQ(ack(v(0), v(2)), v(3));

ML99_ASSERT_EQ(ack(v(1), v(0)), v(2));
ML99_ASSERT_EQ(ack(v(1), v(1)), v(3));
ML99_ASSERT_EQ(ack(v(1), v(2)), v(4));

ML99_ASSERT_EQ(ack(v(2), v(0)), v(3));
ML99_ASSERT_EQ(ack(v(2), v(1)), v(5));
ML99_ASSERT_EQ(ack(v(2), v(2)), v(7));

int main(void) {}
