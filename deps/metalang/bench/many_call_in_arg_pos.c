#include <metalang99.h>

#define F_IMPL(x) v(x)

#define CALL ML99_call(F, ML99_call(F, ML99_call(F, v(~~~~~))))
#define _5   CALL, CALL, CALL, CALL, CALL
#define _10  _5, _5
#define _100 _10, _10, _10, _10, _10, _10, _10, _10, _10, _10

ML99_EVAL(_100)
