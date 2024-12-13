// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(uuidv7);

Datum uuidv7(PG_FUNCTION_ARGS) { PG_RETURN_NULL(); }