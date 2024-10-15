#include <metalang99.h>

#define _10 5, 5, 5, 5, 5, 3, 3, 3, 3, 3
#define _50 _10, _10, _10, _10, _10

#define F_IMPL(x) ML99_if(ML99_natEq(v(x), v(5)), ML99_just(v(x)), ML99_nothing())
#define F_ARITY   1

ML99_LIST_EVAL(ML99_listFilterMap(v(F), ML99_list(v(_50, _10, 3, 3, 3))))
