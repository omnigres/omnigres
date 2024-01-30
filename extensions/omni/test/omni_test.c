#include <dirent.h>
#include <limits.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <commands/dbcommands.h>
#include <miscadmin.h>
#include <utils/builtins.h>

#include <omni.h>

PG_MODULE_MAGIC;
OMNI_MAGIC;

static bool initialized = false;

static char *hello_message;

PG_FUNCTION_INFO_V1(is_backend_initialized);
Datum is_backend_initialized(PG_FUNCTION_ARGS) { PG_RETURN_BOOL(initialized); }

void run_hook_fn(omni_hook_handle *handle, QueryDesc *queryDesc, ScanDirection direction,
                 uint64 count, bool execute_once) {
  ereport(NOTICE, errmsg("run_hook"));
}

void _Omni_load(const omni_handle *handle) {}

static char *shmem_test = NULL;
static char *shmem_test1 = NULL;

static const omni_handle *saved_handle = NULL;

void _Omni_init(const omni_handle *handle) {
  initialized = true;

  omni_hook run_hook = {
      .name = "run_hook", .type = omni_hook_executor_run, .fn = {.executor_run = run_hook_fn}};
  handle->register_hook(handle, &run_hook);

  bool found;

  shmem_test = (char *)handle->allocate_shmem(
      handle, psprintf("test:%s", get_database_name(MyDatabaseId)), 128, &found);

  if (!found) {
    strlcpy(shmem_test, "hello", sizeof("hello"));
  }

  shmem_test1 = (char *)handle->allocate_shmem(
      handle, psprintf("test1:%s", get_database_name(MyDatabaseId)), 128, &found);

  if (!found) {
    strlcpy(shmem_test1, "hello", sizeof("hello"));
  }

  saved_handle = handle; // not always the best idea, but...
}

PG_FUNCTION_INFO_V1(hello);
Datum hello(PG_FUNCTION_ARGS) { PG_RETURN_CSTRING(hello_message); }

PG_FUNCTION_INFO_V1(get_shmem);
Datum get_shmem(PG_FUNCTION_ARGS) { PG_RETURN_CSTRING(shmem_test); }

PG_FUNCTION_INFO_V1(get_shmem1);
Datum get_shmem1(PG_FUNCTION_ARGS) { PG_RETURN_CSTRING(shmem_test1); }

PG_FUNCTION_INFO_V1(set_shmem);
Datum set_shmem(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("should not be null"));
  }
  text_to_cstring_buffer(PG_GETARG_TEXT_PP(0), shmem_test, 127);
  PG_RETURN_CSTRING(shmem_test);
}

PG_FUNCTION_INFO_V1(lookup_shmem);
Datum lookup_shmem(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("should not be null"));
  }
  bool found = false;
  void *allocation =
      saved_handle->lookup_shmem(saved_handle, text_to_cstring(PG_GETARG_TEXT_PP(0)), &found);
  if (!found) {
    PG_RETURN_NULL();
  } else {
    PG_RETURN_CSTRING(allocation); // unsafe in general, but this is a controlled test
  }
}
