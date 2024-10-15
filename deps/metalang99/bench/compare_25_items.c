#include <metalang99.h>

#define NUMBERS                                                                                    \
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25

ML99_ASSERT(ML99_listEq(v(ML99_natEq), ML99_list(v(NUMBERS)), ML99_list(v(NUMBERS))));
