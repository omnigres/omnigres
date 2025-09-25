// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(_uuidv7);

Datum _uuidv7(PG_FUNCTION_ARGS) { PG_RETURN_NULL(); }
