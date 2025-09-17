#include <postgres.h>
#include <utils/guc.h>

#include <omni/omni_v0.h>

PG_MODULE_MAGIC;
OMNI_MAGIC;

OMNI_MODULE_INFO(.name = "omni_kube", .version = EXT_VERSION,
                 .identity = "adbfbf5b-c3c9-4c37-9eb5-8ba226c7e7a4");

void _Omni_init(const omni_handle *handle) {
  // This is defined here because Postgres 14 does not allow us to
  // `grant set on parameter omni_kube.refresh to ...`, so we'll predefine it here
  // and make it user-settable
  omni_guc_variable guc_refresh = {
      .name = "omni_kube.refresh",
      .short_desc = "Indicator of resource table refresh processing (internal)",
      .long_desc = "When set to 'true', resource table is being refreshed",
      .type = PGC_STRING,
      .typed = {.string_val = {.boot_value = ""}},
      .context = PGC_USERSET};
  handle->declare_guc_variable(handle, &guc_refresh);
}
