#include <metalang99.h>

#define F_IMPL(x, y, z) v(x + y + z)

#define _5                                                                                         \
    ML99_call(F, v(1), v(2), v(3)), ML99_call(F, v(1), v(2), v(3)),                                \
        ML99_call(F, v(1), v(2), v(3)), ML99_call(F, v(1), v(2), v(3)),                            \
        ML99_call(F, v(1), v(2), v(3))
#define _10  _5, _5
#define _100 _10, _10, _10, _10, _10, _10, _10, _10, _10, _10

ML99_EVAL(_100)
