#ifndef LIBGLUEPG_STC_COMMON_H
#define LIBGLUEPG_STC_COMMON_H

#include <postgres.h>

#include <libpgaug.h>

#ifndef c_MALLOC
#define c_MALLOC(sz) pgaug_alloc(sz)
#define c_CALLOC(n, sz) pgaug_calloc(n, sz)
#define c_REALLOC(p, sz) pgaug_realloc(p, sz)
#define c_FREE(p) pgaug_free(p)
#endif

#endif // LIBGLUEPG_STC_COMMON_H