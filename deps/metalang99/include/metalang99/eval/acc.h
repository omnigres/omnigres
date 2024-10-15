#ifndef ML99_EVAL_ACC_H
#define ML99_EVAL_ACC_H

#include <metalang99/priv/util.h>

#define ML99_PRIV_EVAL_ACC           (, )
#define ML99_PRIV_EVAL_ACC_COMMA_SEP ()

#define ML99_PRIV_EVAL_0fspace(acc, ...) (ML99_PRIV_EXPAND acc __VA_ARGS__)
#define ML99_PRIV_EVAL_0fcomma(acc, ...) (ML99_PRIV_EXPAND acc, __VA_ARGS__)

#define ML99_PRIV_EVAL_ACC_UNWRAP(_emptiness, ...) __VA_ARGS__

#endif // ML99_EVAL_ACC_H
