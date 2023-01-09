#include <dirent.h>
#include <limits.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <access/heapam.h>
#include <access/sdir.h>
#include <access/table.h>
#include <access/tableam.h>
#include <access/xact.h>
#include <catalog/pg_database.h>
#include <common/hashfn.h>
#include <miscadmin.h>
#include <port.h>
#include <postmaster/bgworker.h>
#include <postmaster/interrupt.h>
#include <storage/fd.h>
#include <storage/ipc.h>
#include <storage/latch.h>
#include <storage/lwlock.h>
#include <storage/shmem.h>
#include <tcop/utility.h>
#include <utils/dsa.h>
#include <utils/guc.h>
#include <utils/hsearch.h>
#include <utils/rel.h>
#include <utils/wait_event.h>

#include <libpgaug.h>

#include <dynpgext.h>

#include "db.h"
#include "omni_ext.h"
#include "workers.h"

typedef struct {
  BackgroundWorkerHandle *worker;
} db_info;

#define i_tag db
#define i_key Oid
#define i_val db_info
#include <stc/cmap.h>

void master_worker(Datum main_arg) {
  BackgroundWorkerInitializeConnection(NULL, NULL, 0);

  pqsignal(SIGHUP, SignalHandlerForConfigReload);
  pqsignal(SIGTERM, die);

  BackgroundWorkerUnblockSignals();

  cmap_db databases = cmap_db_init();

  StartTransactionCommand();

  while (true) {
    CHECK_FOR_INTERRUPTS();

    Relation rel = table_open(DatabaseRelationId, AccessShareLock);
    TableScanDesc scan = table_beginscan_catalog(rel, 0, NULL);
    for (;;) {
      HeapTuple tup = heap_getnext(scan, ForwardScanDirection);
      if (tup == NULL)
        break;
      Form_pg_database db = (Form_pg_database)GETSTRUCT(tup);
      if (db->datistemplate || !db->datallowconn)
        continue;
      db_info dbi;
      cmap_db_result result = cmap_db_insert(&databases, db->oid, dbi);
      if (result.inserted) {
        BackgroundWorker worker = {.bgw_flags =
                                       BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION,
                                   .bgw_main_arg = ObjectIdGetDatum(db->oid),
                                   .bgw_start_time = BgWorkerStart_RecoveryFinished,
                                   .bgw_restart_time = 0,
                                   .bgw_function_name = "database_worker",
                                   .bgw_notify_pid = MyProcPid};
        strncpy(worker.bgw_library_name, MyBgworkerEntry->bgw_library_name, BGW_MAXLEN);
        char *name = MemoryContextStrdup(TopMemoryContext,
                                         psprintf("omni_ext worker (%s)", db->datname.data));
        strncpy(worker.bgw_name, name, BGW_MAXLEN);
        strncpy(worker.bgw_type, name, BGW_MAXLEN);
        strncpy(worker.bgw_extra, db->datname.data, BGW_EXTRALEN);
        RegisterDynamicBackgroundWorker(&worker, &result.ref->second.worker);
        pid_t worker_pid;
        switch (WaitForBackgroundWorkerStartup(result.ref->second.worker, &worker_pid)) {
        case BGWH_STARTED:
          ereport(LOG, errmsg("Started omni_ext database worker for %s (pid %d)", db->datname.data,
                              worker_pid));
          break;
        case BGWH_STOPPED:
          ereport(ERROR,
                  errmsg("Failed to start omni_ext database worker for %s", db->datname.data));
          break;
        case BGWH_POSTMASTER_DIED:
        case BGWH_NOT_YET_STARTED:
          break;
        }
      }
    }
    if (scan->rs_rd->rd_tableam->scan_end) {
      scan->rs_rd->rd_tableam->scan_end(scan);
    }
    table_close(rel, AccessShareLock);

    (void)WaitLatch(MyLatch, WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH, 1000L,
                    PG_WAIT_EXTENSION);
    ResetLatch(MyLatch);
  }
  AbortCurrentTransaction();

  cmap_db_drop(&databases);
}

HASHCTL worker_rendezvous_ctl = {.keysize = sizeof(uint32),
                                 .entrysize = sizeof(database_worker_rendezvous)};

void database_worker(Datum db_oid) {
  ensure_dsa_attached();
  BackgroundWorkerInitializeConnectionByOid(db_oid, InvalidOid, 0);

  pqsignal(SIGHUP, SignalHandlerForConfigReload);
  pqsignal(SIGTERM, die);

  BackgroundWorkerUnblockSignals();

  StartTransactionCommand();

  // Get extensions created in this database
  cmap_extensions extensions = created_extensions();

  HTAB *rendezvous_tab = ShmemInitHash("omni_ext_worker_rendezvous", 0, max_databases,
                                       &worker_rendezvous_ctl, HASH_ELEM | HASH_ATTACH);
  LWLock *lock = &(GetNamedLWLockTranche("omni_ext_worker_rendezvous")->lock);

  // For every dynpgext handle
  c_FOREACH(handle, cdeq_handle, handles) {
    uint32 extension_hash =
        string_hash((*handle.ref)->library_name, strlen((*handle.ref)->library_name));

    // Initialize the rendezvous entry for this extension if we're the first one
    {
      LWLockAcquire(lock, LW_EXCLUSIVE);
      bool extension_rendezvous_found = false;
      database_worker_rendezvous *rendezvous = (database_worker_rendezvous *)hash_search(
          rendezvous_tab, &extension_hash, HASH_ENTER, &extension_rendezvous_found);
      if (!extension_rendezvous_found) {
        rendezvous->used = false;
        rendezvous->global_worker_starter = 0;
      }
      LWLockRelease(lock);
    }

    extension_key key = {.extname = (*handle.ref)->name, .extversion = (*handle.ref)->version};

    // If this database has this extensions
    if (cmap_extensions_contains(&extensions, key)) {

      // Process background worker requests
      c_FOREACH(bgw, cdeq_background_worker_request, background_worker_requests) {
        // Handles must match
        if (bgw.ref->handle != *(handle.ref))
          continue;

        if ((bgw.ref->flags & DYNPGEXT_REGISTER_BGWORKER_NOTIFY) ==
            DYNPGEXT_REGISTER_BGWORKER_NOTIFY) {
          // Ensure this process gets the notifications as it'll do the callback as well
          bgw.ref->bgw.bgw_notify_pid = MyProcPid;
        }

        bool global_background_worker = (bgw.ref->flags & DYNPGEXT_SCOPE_DATABASE_LOCAL) == 0;
        bool start = true;
        if (global_background_worker) {
          // If this worker is not database-local (aka global), ensure we're the only ones starting
          // it
          LWLockAcquire(lock, LW_EXCLUSIVE);
          bool extension_rendezvous_found = false;
          database_worker_rendezvous *rendezvous = (database_worker_rendezvous *)hash_search(
              rendezvous_tab, &extension_hash, HASH_ENTER, &extension_rendezvous_found);
          if (extension_rendezvous_found) {
            // If we are the one to start (or can self-appoint ourselves to)
            if (rendezvous->global_worker_starter == 0 ||
                rendezvous->global_worker_starter == MyProcPid) {
              rendezvous->global_worker_starter = MyProcPid;
              // Then start it
            } else {
              // Otherwise don't
              start = false;
            }
          }
          LWLockRelease(lock);
        }
        if (start) {
          BackgroundWorkerHandle *handle;
          if (!global_background_worker) {
            bgw.ref->bgw.bgw_main_arg = db_oid;
          }
          RegisterDynamicBackgroundWorker(&bgw.ref->bgw, &handle);
          if (bgw.ref->callback) {
            bgw.ref->callback(handle, bgw.ref->data);
          }
        }
      }

      // Update the rendezvous entry for this extension to reflect its use
      {
        LWLockAcquire(lock, LW_EXCLUSIVE);
        bool extension_rendezvous_found = false;
        database_worker_rendezvous *rendezvous = (database_worker_rendezvous *)hash_search(
            rendezvous_tab, &extension_hash, HASH_FIND, &extension_rendezvous_found);
        if (extension_rendezvous_found) {
          rendezvous->used = true;
        }
        LWLockRelease(lock);
      }
    }
  }

  cmap_extensions_drop(&extensions);

  while (true) {
    (void)WaitLatch(MyLatch, WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH, 1000L,
                    PG_WAIT_EXTENSION);
    ResetLatch(MyLatch);

    CHECK_FOR_INTERRUPTS();
  }

  AbortCurrentTransaction();
}