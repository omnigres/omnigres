#include <dirent.h>
#include <limits.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <access/xact.h>
#if PG_MAJORVERSION_NUM < 13
#include <catalog/pg_type.h>
#endif
#include <executor/spi.h>
#include <utils/snapmgr.h>
#include <utils/timestamp.h>

#include <dynpgext.h>

PG_MODULE_MAGIC;
DYNPGEXT_MAGIC;

#ifdef NO_PRELOAD
bool _Dynpgext_eager_preload() { return false; }
#define GLOBAL_ID "omni_ext_test_no_preload:global"
#define DATABASE_LOCAL_ID "omni_ext_test_no_preload:dblocal"
#else
#define GLOBAL_ID "omni_ext_test:global"
#define DATABASE_LOCAL_ID "omni_ext_test:dblocal"
#endif

int db_counter = 0;

void cb(void *ptr, void *data) { strcpy((char *)ptr, "test"); }
void cb_dblocal(void *ptr, void *data) { sprintf((char *)ptr, "testdb %d", db_counter++); }

void global_worker(Datum arg) {
  BackgroundWorkerInitializeConnection("omni_ext_test", NULL, 0);

  SetCurrentStatementStartTimestamp();
  StartTransactionCommand();
  PushActiveSnapshot(GetTransactionSnapshot());

  SPI_connect();
  SPI_execute("CREATE TABLE IF NOT EXISTS global_worker_started ();", false, 0);
  SPI_finish();

  PopActiveSnapshot();
  CommitTransactionCommand();
}

PG_FUNCTION_INFO_V1(wait_for_table);

Datum wait_for_table(PG_FUNCTION_ARGS) {
  TimestampTz start = GetCurrentTimestamp();
  bool found = false;
  SPI_connect();
  while (true) {
    int res =
        SPI_execute_with_args("SELECT table_name FROM information_schema.tables "
                              "WHERE table_name = $1",
                              1, (Oid[1]){TEXTOID}, (Datum[1]){PG_GETARG_DATUM(0)}, " ", false, 1);
    if (res == SPI_OK_SELECT) {
      if (SPI_processed == 1) {
        found = true;
        break;
      }
    }
    TimestampTz now = GetCurrentTimestamp();
    // timeout
    if (now - start >= 3000000)
      break;
  }
  SPI_finish();
  PG_RETURN_BOOL(found);
}

void database_local_worker(Datum dboid) {
  BackgroundWorkerInitializeConnectionByOid(dboid, InvalidOid, 0);

  SetCurrentStatementStartTimestamp();
  StartTransactionCommand();
  PushActiveSnapshot(GetTransactionSnapshot());

  SPI_connect();
  SPI_execute("CREATE TABLE IF NOT EXISTS local_worker_started ();", false, 0);
  SPI_finish();

  PopActiveSnapshot();
  CommitTransactionCommand();
}

void _Dynpgext_init(const dynpgext_handle *handle) {
  ereport(NOTICE, errmsg("_Dynpgext_init"));
  handle->allocate_shmem(handle, GLOBAL_ID, 1024, cb, NULL, DYNPGEXT_SCOPE_GLOBAL);
  handle->allocate_shmem(handle, DATABASE_LOCAL_ID, 1024, cb_dblocal, NULL,
                         DYNPGEXT_SCOPE_DATABASE_LOCAL);
  // Global background worker
  BackgroundWorker bgw = {.bgw_name = "omni_ext_test global",
                          .bgw_type = "omni_ext_test",
                          .bgw_function_name = "global_worker",
                          .bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION,
                          .bgw_start_time = BgWorkerStart_RecoveryFinished,
                          .bgw_restart_time = BGW_NEVER_RESTART};
  strncpy(bgw.bgw_library_name, handle->library_name, BGW_MAXLEN);
  handle->register_bgworker(handle, &bgw, NULL, NULL,
                            DYNPGEXT_REGISTER_BGWORKER_NOTIFY | DYNPGEXT_SCOPE_GLOBAL);

  // Database-local background worker
  BackgroundWorker bgwl = {.bgw_name = "omni_ext_test local",
                           .bgw_type = "omni_ext_test",
                           .bgw_function_name = "database_local_worker",
                           .bgw_flags =
                               BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION,
                           .bgw_start_time = BgWorkerStart_RecoveryFinished,
                           .bgw_restart_time = BGW_NEVER_RESTART};
  strncpy(bgwl.bgw_library_name, handle->library_name, BGW_MAXLEN);
  handle->register_bgworker(handle, &bgwl, NULL, NULL,
                            DYNPGEXT_REGISTER_BGWORKER_NOTIFY | DYNPGEXT_SCOPE_DATABASE_LOCAL);
}

PG_FUNCTION_INFO_V1(alloc_shmem_global);

Datum alloc_shmem_global(PG_FUNCTION_ARGS) {
  char *test = dynpgext_lookup_shmem(GLOBAL_ID);
  if (test) {
    PG_RETURN_CSTRING(test);
  } else {
    ereport(ERROR, errmsg("no allocation found"));
  }
}

PG_FUNCTION_INFO_V1(alloc_shmem_database_local);

Datum alloc_shmem_database_local(PG_FUNCTION_ARGS) {
  char *test = dynpgext_lookup_shmem(DATABASE_LOCAL_ID);
  if (test) {
    PG_RETURN_CSTRING(test);
  } else {
    ereport(ERROR, errmsg("no allocation found"));
  }
}

PG_FUNCTION_INFO_V1(update_global_value);

Datum update_global_value(PG_FUNCTION_ARGS) {
  void *ptr = (char *)dynpgext_lookup_shmem(GLOBAL_ID);
  char *value = PG_GETARG_CSTRING(0);
  strncpy(ptr, value, Min(strlen(value), 1024));
  PG_RETURN_CSTRING(value);
}