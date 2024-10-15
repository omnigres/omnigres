#ifndef ML99_PRIV_BOOL_H
#define ML99_PRIV_BOOL_H

#define ML99_PRIV_TRUE(...)  1
#define ML99_PRIV_FALSE(...) 0

#define ML99_PRIV_NOT(b) ML99_PRIV_LOGICAL_OVERLOAD_SINGLE(ML99_PRIV_NOT_, b)
#define ML99_PRIV_NOT_0  1
#define ML99_PRIV_NOT_1  0

#define ML99_PRIV_AND(x, y) ML99_PRIV_LOGICAL_OVERLOAD(ML99_PRIV_AND_, x, y)
#define ML99_PRIV_AND_00    0
#define ML99_PRIV_AND_01    0
#define ML99_PRIV_AND_10    0
#define ML99_PRIV_AND_11    1

#define ML99_PRIV_OR(x, y) ML99_PRIV_LOGICAL_OVERLOAD(ML99_PRIV_OR_, x, y)
#define ML99_PRIV_OR_00    0
#define ML99_PRIV_OR_01    1
#define ML99_PRIV_OR_10    1
#define ML99_PRIV_OR_11    1

#define ML99_PRIV_OR3(a, b, c)    ML99_PRIV_OR(a, ML99_PRIV_OR(b, c))
#define ML99_PRIV_OR4(a, b, c, d) ML99_PRIV_OR3(a, b, ML99_PRIV_OR(c, d))

#define ML99_PRIV_XOR(x, y) ML99_PRIV_LOGICAL_OVERLOAD(ML99_PRIV_XOR_, x, y)
#define ML99_PRIV_XOR_00    0
#define ML99_PRIV_XOR_01    1
#define ML99_PRIV_XOR_10    1
#define ML99_PRIV_XOR_11    0

#define ML99_PRIV_BOOL_EQ(x, y) ML99_PRIV_LOGICAL_OVERLOAD(ML99_PRIV_BOOL_EQ_, x, y)
#define ML99_PRIV_BOOL_EQ_00    1
#define ML99_PRIV_BOOL_EQ_01    0
#define ML99_PRIV_BOOL_EQ_10    0
#define ML99_PRIV_BOOL_EQ_11    1

#define ML99_PRIV_LOGICAL_OVERLOAD(op, x, y)     op##x##y
#define ML99_PRIV_LOGICAL_OVERLOAD_SINGLE(op, b) op##b

#define ML99_PRIV_IF(cond, x, y)    ML99_PRIV_IF_OVERLOAD(cond)(x, y)
#define ML99_PRIV_IF_OVERLOAD(cond) ML99_PRIV_IF_##cond
#define ML99_PRIV_IF_0(_x, y)       y
#define ML99_PRIV_IF_1(x, _y)       x

#endif // ML99_PRIV_BOOL_H
