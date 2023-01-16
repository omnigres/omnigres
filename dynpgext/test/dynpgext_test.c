#include <dirent.h>
#include <limits.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <utils/elog.h>

#include <dynpgext.h>

PG_MODULE_MAGIC;
DYNPGEXT_MAGIC;

PG_FUNCTION_INFO_V1(loader_present);
Datum loader_present(PG_FUNCTION_ARGS) { PG_RETURN_BOOL(dynpgext_loader_present()); }
