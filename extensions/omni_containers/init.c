// clang-format off
#include <postgres.h>
#include <utils/memutils.h>
#include <utils/guc.h>
// clang-format on

#include <libgluepg_curl.h>

#include "omni_containers.h"

#ifdef DEBUG
char *test_fixtures = NULL;
#endif

void _PG_init() {
#ifdef DEBUG
  DefineCustomStringVariable("omni_containers.fixtures", "Test fixtures", NULL,
                             &test_fixtures, NULL, PGC_USERSET, 0, NULL, NULL,
                             NULL);
#endif
}
