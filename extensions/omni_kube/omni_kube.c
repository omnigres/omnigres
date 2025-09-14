#include <postgres.h>
#include <utils/guc.h>

PG_MODULE_MAGIC;

static char *omni_kube_refresh = NULL;

PG_FUNCTION_INFO_V1(dummy);
Datum dummy(PG_FUNCTION_ARGS) { PG_RETURN_NULL(); }

void _PG_init(void) {
  // This is defined here because Postgres 14 does not allow us to
  // `grant set on parameter omni_kube.refresh to ...`, so we'll predefine it here
  // and make it user-settable
  DefineCustomStringVariable("omni_kube.refresh",
                             "Indicator of resource table refresh processing (internal)",
                             "When set to 'true', resource table is being refreshed",
                             &omni_kube_refresh, "", PGC_USERSET, 0, NULL, NULL, NULL);
}
