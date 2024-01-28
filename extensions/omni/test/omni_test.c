#include <dirent.h>
#include <limits.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <omni.h>

PG_MODULE_MAGIC;
OMNI_MAGIC;

static bool initialized = false;

static char *hello_message = "hello";

PG_FUNCTION_INFO_V1(is_backend_initialized);
Datum is_backend_initialized(PG_FUNCTION_ARGS) { PG_RETURN_BOOL(initialized); }

void run_hook_fn(omni_hook_handle *handle, QueryDesc *queryDesc, ScanDirection direction,
                 uint64 count, bool execute_once) {
  ereport(NOTICE, errmsg("run_hook"));
}

void _Omni_init(const omni_handle *handle) {
  initialized = true;

  omni_hook run_hook = {
      .name = "run_hook", .type = omni_hook_executor_run, .fn = {.executor_run = run_hook_fn}};
  handle->register_hook(handle, &run_hook);
}

PG_FUNCTION_INFO_V1(hello);
Datum hello(PG_FUNCTION_ARGS) { PG_RETURN_CSTRING(hello_message); }