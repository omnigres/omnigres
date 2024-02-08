#include <dirent.h>
#include <limits.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <commands/dbcommands.h>
#include <miscadmin.h>
#include <storage/latch.h>
#include <utils/builtins.h>
#if PG_MAJORVERSION_NUM >= 14
#include <utils/wait_event.h>
#else
#include <pgstat.h>
#endif

#include <omni/omni_utils.h>
#include <omni/omni_v0.h>

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

void _Omni_load(const omni_handle *handle) {
  {
    // Register a global dynamic background worker
    BackgroundWorker master_worker = {.bgw_name = "test_global_worker",
                                      .bgw_type = "test_global_worker",
                                      .bgw_flags = BGWORKER_SHMEM_ACCESS |
                                                   BGWORKER_BACKEND_DATABASE_CONNECTION,
                                      .bgw_start_time = BgWorkerStart_RecoveryFinished,
                                      .bgw_restart_time = BGW_NEVER_RESTART,
                                      .bgw_function_name = "test_worker",
                                      .bgw_notify_pid = 0};
    strncpy(master_worker.bgw_library_name, handle->get_library_name(handle), BGW_MAXLEN);
    RegisterDynamicBackgroundWorker(&master_worker, NULL);
  }
}

void test_worker(Datum id) {
  pqsignal(SIGTERM, die);
  BackgroundWorkerInitializeConnection(NULL, NULL, 0);
  BackgroundWorkerUnblockSignals();
  while (true) {
    // And then waiting
    (void)WaitLatch(MyLatch, WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH, 1000L,
                    PG_WAIT_EXTENSION);
    ResetLatch(MyLatch);

    CHECK_FOR_INTERRUPTS();
  }
}

static char *shmem_test = NULL;
static char *shmem_test1 = NULL;

static const omni_handle *saved_handle = NULL;

static OmniBackgroundWorkerHandle *local_bgw_handle = NULL;

bool *GUC_bool;
int *GUC_int;
char **GUC_string;
double *GUC_real;
int *GUC_enum;

const struct config_enum_entry GUC_enum_options[3] = {
    {.name = "test", .val = 1}, {.name = "test1", .val = 2}, {.name = NULL}};

static void register_bgworker(void *ptr, void *arg) {
  const omni_handle *handle = (const omni_handle *)arg;
  OmniBackgroundWorkerHandle *bgw_handle = (OmniBackgroundWorkerHandle *)ptr;
  BackgroundWorker worker = {.bgw_name = "test_local_worker",
                             .bgw_type = "test_local_worker",
                             .bgw_flags =
                                 BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION,
                             .bgw_start_time = BgWorkerStart_RecoveryFinished,
                             .bgw_restart_time = BGW_NEVER_RESTART,
                             .bgw_function_name = "test_worker",
                             .bgw_main_arg = ObjectIdGetDatum(MyDatabaseId),
                             .bgw_notify_pid = MyProcPid};
  strncpy(worker.bgw_library_name, handle->get_library_name(handle), BGW_MAXLEN);
  OmniRegisterDynamicBackgroundWorker(&worker, bgw_handle);
  pid_t pid;
  WaitForBackgroundWorkerStartup(GetBackgroundWorkerHandle(bgw_handle), &pid);
}

void _Omni_init(const omni_handle *handle) {
  initialized = true;

  omni_hook run_hook = {
      .name = "run_hook", .type = omni_hook_executor_run, .fn = {.executor_run = run_hook_fn}};
  handle->register_hook(handle, &run_hook);

  bool found;

  shmem_test =
      (char *)handle->allocate_shmem(handle, psprintf("test:%s", get_database_name(MyDatabaseId)),
                                     128, (void (*)(void *, void *))strcpy, "hello", &found);

  shmem_test1 =
      (char *)handle->allocate_shmem(handle, psprintf("test1:%s", get_database_name(MyDatabaseId)),
                                     128, (void (*)(void *, void *))strcpy, "hello", &found);

  {
    // Register a per-database worker

    // Let's use allocator's lock to prevent others from starting
    bool found;
    local_bgw_handle = (OmniBackgroundWorkerHandle *)handle->allocate_shmem(
        handle, psprintf("workers:%s", get_database_name(MyDatabaseId)),
        sizeof(OmniBackgroundWorkerHandle), register_bgworker, (void *)handle, &found);
  }

  omni_guc_variable guc_bool = {.name = "omni_test.bool",
                                .type = PGC_BOOL,
                                .typed = {.bool_val = {.boot_value = false}},
                                .context = PGC_USERSET};
  handle->declare_guc_variable(handle, &guc_bool);
  GUC_bool = guc_bool.typed.bool_val.value;

  omni_guc_variable guc_int = {
      .name = "omni_test.int",
      .type = PGC_INT,
      .typed = {.int_val = {.boot_value = 1, .min_value = 1, .max_value = INT_MAX}},
      .context = PGC_USERSET};
  handle->declare_guc_variable(handle, &guc_int);
  GUC_int = guc_int.typed.int_val.value;

  omni_guc_variable guc_real = {
      .name = "omni_test.real",
      .type = PGC_REAL,
      .typed = {.real_val = {.boot_value = 1.23, .min_value = 1, .max_value = 100}},
      .context = PGC_USERSET};
  handle->declare_guc_variable(handle, &guc_real);
  GUC_real = guc_real.typed.real_val.value;

  omni_guc_variable guc_string = {.name = "omni_test.string",
                                  .type = PGC_STRING,
                                  .typed = {.string_val = {.boot_value = ""}},
                                  .context = PGC_USERSET};
  handle->declare_guc_variable(handle, &guc_string);
  GUC_string = guc_string.typed.string_val.value;

  omni_guc_variable guc_enum = {
      .name = "omni_test.enum",
      .type = PGC_ENUM,
      .typed = {.enum_val = {.boot_value = 1, .options = GUC_enum_options}},
      .context = PGC_USERSET};
  handle->declare_guc_variable(handle, &guc_enum);
  GUC_enum = guc_enum.typed.enum_val.value;

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

PG_FUNCTION_INFO_V1(local_worker_pid);
Datum local_worker_pid(PG_FUNCTION_ARGS) {
  pid_t pid = 0;
  GetBackgroundWorkerPid(GetBackgroundWorkerHandle(local_bgw_handle), &pid);
  PG_RETURN_INT32(pid);
}

PG_FUNCTION_INFO_V1(guc_int);
Datum guc_int(PG_FUNCTION_ARGS) { PG_RETURN_INT32(*GUC_int); }

PG_FUNCTION_INFO_V1(guc_bool);
Datum guc_bool(PG_FUNCTION_ARGS) { PG_RETURN_BOOL(*GUC_bool); }

PG_FUNCTION_INFO_V1(guc_real);
Datum guc_real(PG_FUNCTION_ARGS) { PG_RETURN_FLOAT8(*GUC_real); }

PG_FUNCTION_INFO_V1(guc_string);
Datum guc_string(PG_FUNCTION_ARGS) { PG_RETURN_CSTRING(*GUC_string); }

PG_FUNCTION_INFO_V1(guc_enum);
Datum guc_enum(PG_FUNCTION_ARGS) { PG_RETURN_INT32(*GUC_enum); }
