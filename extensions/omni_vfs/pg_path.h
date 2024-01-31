// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#if PG_MAJORVERSION_NUM < 15
char *make_absolute_path_15(const char *path);
char *canonicalize_path_15(const char *path);
#define make_absolute_path(p) make_absolute_path_15(p)
#define canonicalize_path(p) canonicalize_path_15(p)
#endif
