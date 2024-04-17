// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <utils/builtins.h>

#include <omni/omni_v0.h>

#include "hooks.h"

bool hook_called[__OMNI_HOOK_TYPE_COUNT] = {};

void xact_callback(omni_hook_handle *handle, XactEvent event) {
  hook_called[omni_hook_xact_callback] = true;
}

void planner_hook_fn(omni_hook_handle *handle, Query *parse, const char *query_string,
                     int cursorOptions, ParamListInfo boundParams) {
  hook_called[omni_hook_planner] = true;
}

#include "../hook_types.h"

PG_FUNCTION_INFO_V1(was_hook_called);

Datum was_hook_called(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("name is required"));
  }
  char *hook_type = text_to_cstring(PG_GETARG_TEXT_PP(0));
  for (int i = 0; i < __OMNI_HOOK_TYPE_COUNT; i++) {
    if (omni_hook_types[i] && strcmp(hook_type, omni_hook_types[i]) == 0) {
      PG_RETURN_BOOL(hook_called[i]);
    }
  }
  PG_RETURN_BOOL(false);
}
