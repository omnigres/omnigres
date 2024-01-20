#include <dirent.h>
#include <limits.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <access/heapam.h>
#include <access/table.h>
#include <access/tableam.h>
#include <access/xact.h>
#include <catalog/pg_database.h>
#include <catalog/pg_extension.h>
#if PG_MAJORVERSION_NUM >= 13
#include <common/hashfn.h>
#endif
#include <miscadmin.h>
#include <port.h>
#include <postmaster/bgworker.h>
#if PG_MAJORVERSION_NUM >= 13
#include <postmaster/interrupt.h>
#endif
#include <utils/builtins.h>

#if PG_MAJORVERSION_NUM >= 14
#include <utils/wait_event.h>
#else
#include <pgstat.h>
#endif

#include <libpgaug.h>

#include <libgluepg_stc.h>

#include <dynpgext.h>

#include "omni_ext.h"
#include "workers.h"

typedef struct {
  BackgroundWorkerHandle *worker;
  pid_t pid;
  char dbname[NAMEDATALEN];
} db_info;

static void sigterm(SIGNAL_ARGS) {}

void master_worker(Datum main_arg) {
  BackgroundWorkerInitializeConnection(NULL, NULL, 0);

  pqsignal(SIGTERM, sigterm);

  BackgroundWorkerUnblockSignals();

  StartTransactionCommand();

  Relation rel = table_open(DatabaseRelationId, AccessShareLock);
  TableScanDesc scan = table_beginscan_catalog(rel, 0, NULL);
  for (;;) {
    HeapTuple tup = heap_getnext(scan, ForwardScanDirection);
    if (tup == NULL)
      break;
    Form_pg_database db = (Form_pg_database)GETSTRUCT(tup);
    if (db->datistemplate || !db->datallowconn)
      continue;
    db_info dbi = {.pid = 0};
    strncpy(dbi.dbname, db->datname.data, sizeof(dbi.dbname));
    populate_bgworker_requests_for_db(db->oid);

    // Start the database worker
    BackgroundWorker worker = {.bgw_flags =
                                   BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION,
                               .bgw_main_arg = ObjectIdGetDatum(db->oid),
                               .bgw_start_time = BgWorkerStart_RecoveryFinished,
                               .bgw_restart_time = BGW_NEVER_RESTART,
                               .bgw_function_name = "database_worker",
                               .bgw_notify_pid = MyProcPid};
    strncpy(worker.bgw_library_name, get_library_name(), BGW_MAXLEN);
    char *name =
        MemoryContextStrdup(TopMemoryContext, psprintf("omni_ext worker (%s)", dbi.dbname));
    strncpy(worker.bgw_name, name, BGW_MAXLEN);
    strncpy(worker.bgw_type, name, BGW_MAXLEN);
    strncpy(worker.bgw_extra, db->datname.data, BGW_EXTRALEN);

    bool ok = RegisterDynamicBackgroundWorker(&worker, NULL);

    if (!ok) {
      ereport(FATAL, errmsg("Can't register a dynamic background worker"));
    }
  }
  if (scan->rs_rd->rd_tableam->scan_end) {
    scan->rs_rd->rd_tableam->scan_end(scan);
  }
  table_close(rel, AccessShareLock);

  // Ensure we're not getting a XID
  Assert(GetCurrentTransactionIdIfAny() == InvalidTransactionId);
  AbortCurrentTransaction();
}

void database_worker(Datum db_oid) {
  ensure_dsa_attached();
  BackgroundWorkerInitializeConnectionByOid(db_oid, InvalidOid, 0);

  pqsignal(SIGTERM, sigterm);

  BackgroundWorkerUnblockSignals();

  StartTransactionCommand();

  Relation rel = table_open(ExtensionRelationId, AccessShareLock);
  TableScanDesc scan = table_beginscan_catalog(rel, 0, NULL);
  for (;;) {
    HeapTuple tup = heap_getnext(scan, ForwardScanDirection);
    if (tup == NULL)
      break;
    Form_pg_extension ext = (Form_pg_extension)GETSTRUCT(tup);

    bool is_version_null;
    Datum version_datum = heap_getattr(tup, 6, rel->rd_att, &is_version_null);

    text *version = is_version_null ? NULL : DatumGetTextPP(version_datum);

    char *cversion = text_to_cstring(version);
    process_extensions_for_database(NameStr(ext->extname), cversion, MyDatabaseId);
    pfree(cversion);
  }

  if (scan->rs_rd->rd_tableam->scan_end) {
    scan->rs_rd->rd_tableam->scan_end(scan);
  }
  table_close(rel, AccessShareLock);

  // Ensure we're not getting a XID
  Assert(GetCurrentTransactionIdIfAny() == InvalidTransactionId);
  AbortCurrentTransaction();
}