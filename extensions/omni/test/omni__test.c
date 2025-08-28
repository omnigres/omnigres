#include <dirent.h>
#include <limits.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <commands/dbcommands.h>
#include <executor/spi.h>
#include <miscadmin.h>
#include <storage/latch.h>
#include <storage/lwlock.h>
#include <utils/builtins.h>
#include <utils/lsyscache.h>
#if PG_MAJORVERSION_NUM >= 14
#include <utils/wait_event.h>
#else
#include <pgstat.h>
#endif

#include <omni/omni_v0.h>

#include "hooks.h"

PG_MODULE_MAGIC;
OMNI_MAGIC;

OMNI_MODULE_INFO(.name = "omni__test", .version = EXT_VERSION,
                 .identity = "ed0aaa35-54c6-426e-a69d-2c74a836053b");

static bool omni_loaded = false;

void _PG_init() { omni_loaded = omni_is_present(); }

PG_FUNCTION_INFO_V1(omni_present_test);

Datum omni_present_test(PG_FUNCTION_ARGS) { PG_RETURN_BOOL(omni_loaded); }

static bool initialized = false;

static char *hello_message;

PG_FUNCTION_INFO_V1(is_backend_initialized);
Datum is_backend_initialized(PG_FUNCTION_ARGS) { PG_RETURN_BOOL(initialized); }

void run_hook_fn(omni_hook_handle *handle, QueryDesc *queryDesc, ScanDirection direction,
                 uint64 count, bool execute_once) {
  ereport(NOTICE, errmsg("run_hook"));
}

#define GLOBAL_WORKER_STARTED 1 << 0
#define LOCK_REGISTRATION 1 << 1

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

static omni_bgworker_handle *local_bgw_handle = NULL;

bool *GUC_bool;
int *GUC_int;
char **GUC_string;
double *GUC_real;
int *GUC_enum;

const struct config_enum_entry GUC_enum_options[3] = {
    {.name = "test", .val = 1}, {.name = "test1", .val = 2}, {.name = NULL}};

static void register_bgworker(const omni_handle *handle, void *ptr, void *arg, bool allocated) {
  if (allocated) {
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
    omni_bgworker_handle *ptr_handle = (omni_bgworker_handle *)ptr;
    handle->request_bgworker_start(handle, &worker, ptr_handle,
                                   (omni_bgworker_options){.timing = omni_timing_immediately});
  }
}

static LWLock *mylock;

static void init_strcpy(const omni_handle *handle, void *ptr, void *data, bool allocated) {
  if (allocated) {
    strcpy((char *)ptr, (char *)data);
  }
}

void _Omni_init(const omni_handle *handle) {
  {
    SPI_connect();
    SPI_execute("create table if not exists public.events ("
                "    event text,"
                "    ts    timestamp default clock_timestamp()"
                ")",
                false, 0);
    SPI_execute("insert into public.events (event) values ('init')", false, 0);
    SPI_finish();
  }

  initialized = true;

  if ((handle->atomic_switch(handle, omni_switch_on, 0, GLOBAL_WORKER_STARTED) &
       GLOBAL_WORKER_STARTED) == GLOBAL_WORKER_STARTED) {
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

  omni_hook run_hook = {
      .name = "run_hook", .type = omni_hook_executor_run, .fn = {.executor_run = run_hook_fn}};
  handle->register_hook(handle, &run_hook);

  omni_hook xact_callback_hook = {.name = "xact_callback",
                                  .type = omni_hook_xact_callback,
                                  .fn = {.xact_callback = xact_callback}};
  handle->register_hook(handle, &xact_callback_hook);

  omni_hook planner_hook = {
      .name = "planner_hook", .type = omni_hook_planner, .fn = {.planner = planner_hook_fn}};
  handle->register_hook(handle, &planner_hook);

  bool found;

  char *dbname = get_database_name(MyDatabaseId);

  shmem_test = (char *)handle->allocate_shmem(handle, psprintf("test:%s", dbname), 128, init_strcpy,
                                              "hello", &found);

  shmem_test1 = (char *)handle->allocate_shmem(handle, psprintf("test1:%s", dbname), 128,
                                               init_strcpy, "hello", &found);

  handle->atomic_switch(handle, omni_switch_on, 0, LOCK_REGISTRATION);
  mylock = handle->allocate_shmem(handle, "mylock", sizeof(LWLock),
                                  (omni_allocate_shmem_callback_function)handle->register_lwlock,
                                  "mylock", &found);

  {
    // Register a per-database worker

    // Let's use allocator's lock to prevent others from starting
    bool found;
    local_bgw_handle = (omni_bgworker_handle *)handle->allocate_shmem(
        handle, psprintf("workers:%s", dbname), sizeof(*local_bgw_handle), register_bgworker, NULL,
        &found);
  }

  omni_guc_variable guc_bool = {.name = "omni__test.bool",
                                .type = PGC_BOOL,
                                .typed = {.bool_val = {.boot_value = false}},
                                .context = PGC_USERSET};
  handle->declare_guc_variable(handle, &guc_bool);
  GUC_bool = guc_bool.typed.bool_val.value;

  omni_guc_variable guc_int = {
      .name = "omni__test.int",
      .type = PGC_INT,
      .typed = {.int_val = {.boot_value = 1, .min_value = 1, .max_value = INT_MAX}},
      .context = PGC_USERSET};
  handle->declare_guc_variable(handle, &guc_int);
  GUC_int = guc_int.typed.int_val.value;

  omni_guc_variable guc_real = {
      .name = "omni__test.real",
      .type = PGC_REAL,
      .typed = {.real_val = {.boot_value = 1.23, .min_value = 1, .max_value = 100}},
      .context = PGC_USERSET};
  handle->declare_guc_variable(handle, &guc_real);
  GUC_real = guc_real.typed.real_val.value;

  omni_guc_variable guc_string = {.name = "omni__test.string",
                                  .type = PGC_STRING,
                                  .typed = {.string_val = {.boot_value = ""}},
                                  .context = PGC_USERSET};
  handle->declare_guc_variable(handle, &guc_string);
  GUC_string = guc_string.typed.string_val.value;

  omni_guc_variable guc_enum = {
      .name = "omni__test.enum",
      .type = PGC_ENUM,
      .typed = {.enum_val = {.boot_value = 1, .options = GUC_enum_options}},
      .context = PGC_USERSET};
  handle->declare_guc_variable(handle, &guc_enum);
  GUC_enum = guc_enum.typed.enum_val.value;

  saved_handle = handle; // not always the best idea, but...
}

void _Omni_deinit(const omni_handle *handle) {
  {
    SPI_connect();
    SPI_execute("create table if not exists public.events ("
                "    event text,"
                "    ts    timestamp default clock_timestamp()"
                ")",
                false, 0);
    SPI_execute("insert into public.events (event) values ('deinit')", false, 0);
    SPI_finish();
  }
  bool found;

  if ((handle->atomic_switch(handle, omni_switch_off, 0, LOCK_REGISTRATION) & LOCK_REGISTRATION) ==
      LOCK_REGISTRATION) {
    handle->unregister_lwlock(handle, mylock);
  }

  char *dbname = get_database_name(MyDatabaseId);

  handle->deallocate_shmem(handle, psprintf("test:%s", dbname), &found);
  handle->deallocate_shmem(handle, psprintf("test1:%s", dbname), &found);
  handle->deallocate_shmem(handle, "mylock", &found);
  handle->deallocate_shmem(handle, psprintf("workers:%s", dbname), &found);
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
  if (local_bgw_handle->registered) {
    GetBackgroundWorkerPid(&local_bgw_handle->bgw_handle, &pid);
    PG_RETURN_INT32(pid);
  } else {
    PG_RETURN_NULL();
  }
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

PG_FUNCTION_INFO_V1(lock_mylock);
Datum lock_mylock(PG_FUNCTION_ARGS) {
  LWLockAcquire(mylock, LW_EXCLUSIVE);
  PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(unlock_mylock);
Datum unlock_mylock(PG_FUNCTION_ARGS) {
  LWLockRelease(mylock);
  PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(mylock_tranche_id);
Datum mylock_tranche_id(PG_FUNCTION_ARGS) { PG_RETURN_INT16(mylock->tranche); }

PG_FUNCTION_INFO_V1(lwlock_identifier);
Datum lwlock_identifier(PG_FUNCTION_ARGS) {
  PG_RETURN_CSTRING(GetLWLockIdentifier(PG_WAIT_LWLOCK, PG_GETARG_INT16(0)));
}

PG_FUNCTION_INFO_V1(bad_shmalloc);
Datum bad_shmalloc(PG_FUNCTION_ARGS) {
  bool found;
  saved_handle->allocate_shmem(saved_handle, "bad_shmalloc", SIZE_MAX, NULL, NULL, &found);
  PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(alloc_shmem);
Datum alloc_shmem(PG_FUNCTION_ARGS) {
  bool found;
  saved_handle->allocate_shmem(saved_handle, text_to_cstring(PG_GETARG_TEXT_PP(0)),
                               PG_GETARG_INT64(1), NULL, NULL, &found);
  PG_RETURN_BOOL(found);
}

PG_FUNCTION_INFO_V1(dealloc_shmem);
Datum dealloc_shmem(PG_FUNCTION_ARGS) {
  bool found;
  saved_handle->deallocate_shmem(saved_handle, text_to_cstring(PG_GETARG_TEXT_PP(0)), &found);
  PG_RETURN_BOOL(found);
}

PG_FUNCTION_INFO_V1(atomic_on);
Datum atomic_on(PG_FUNCTION_ARGS) {
  uint64 i = saved_handle->atomic_switch(saved_handle, omni_switch_on, 0, PG_GETARG_INT64(0));
  PG_RETURN_INT64(i);
}

PG_FUNCTION_INFO_V1(atomic_off);
Datum atomic_off(PG_FUNCTION_ARGS) {
  uint64 i = saved_handle->atomic_switch(saved_handle, omni_switch_off, 0, PG_GETARG_INT64(0));
  PG_RETURN_INT64(i);
}
