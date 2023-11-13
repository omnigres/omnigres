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
#include <storage/fd.h>
#include <storage/ipc.h>
#include <storage/latch.h>
#include <storage/lwlock.h>
#include <storage/shmem.h>
#include <tcop/utility.h>
#include <utils/builtins.h>
#include <utils/dsa.h>
#include <utils/guc.h>
#include <utils/hsearch.h>
#include <utils/rel.h>

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

#define i_tag db
#define i_key Oid
#define i_val db_info
#include <stc/cmap.h>

static bool terminate = false;
static void sigterm(SIGNAL_ARGS) {
  terminate = true;
  SetLatch(MyLatch);
}

void master_worker(Datum main_arg) {
  BackgroundWorkerInitializeConnection(NULL, NULL, 0);

  pqsignal(SIGTERM, sigterm);

  BackgroundWorkerUnblockSignals();

  cmap_db databases = cmap_db_init();

  while (!terminate) {
    CHECK_FOR_INTERRUPTS();

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
      cmap_db_result result = cmap_db_insert(&databases, db->oid, dbi);
      if (result.inserted) {
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

        // Ensure we allocate worker handles under the top memory context
        MemoryContext old_context = MemoryContextSwitchTo(TopMemoryContext);
        bool ok = RegisterDynamicBackgroundWorker(&worker, &result.ref->second.worker);
        MemoryContextSwitchTo(old_context);

        if (!ok) {
          ereport(FATAL, errmsg("Can't register a dynamic eror"));
        }
      }
    }
    if (scan->rs_rd->rd_tableam->scan_end) {
      scan->rs_rd->rd_tableam->scan_end(scan);
    }
    table_close(rel, AccessShareLock);

    // Ensure we're not getting a XID
    Assert(GetCurrentTransactionIdIfAny() == InvalidTransactionId);
    AbortCurrentTransaction();

    c_FOREACH(worker, cmap_db, databases) {
      db_info *info = &worker.ref->second;
      // If PID of the worker is not known yet
      if (info->pid == 0) {
        switch (GetBackgroundWorkerPid(info->worker, &info->pid)) {
        case BGWH_STARTED:
          ereport(LOG, errmsg("Started omni_ext database worker for %s (pid %d)", info->dbname,
                              info->pid));
          break;
        case BGWH_STOPPED:
          ereport(ERROR, errmsg("Failed to start omni_ext database worker for %s", info->dbname));
        case BGWH_POSTMASTER_DIED:
        case BGWH_NOT_YET_STARTED:
          break;
        }
      }
    }

    (void)WaitLatch(MyLatch, WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH, 1000L,
                    PG_WAIT_EXTENSION);
    ResetLatch(MyLatch);
  }

  cmap_db_drop(&databases);
}

HASHCTL worker_rendezvous_ctl = {.keysize = sizeof(uint32),
                                 .entrysize = sizeof(database_worker_rendezvous)};

#define i_key Oid
#define i_tag oid
#include <stc/cset.h>

void database_worker(Datum db_oid) {
  ensure_dsa_attached();
  BackgroundWorkerInitializeConnectionByOid(db_oid, InvalidOid, 0);

  pqsignal(SIGTERM, sigterm);

  BackgroundWorkerUnblockSignals();

  // This is how workers communicate initialization of extensions between each other
  HTAB *rendezvous_tab =
      ShmemInitHash("omni_ext_worker_rendezvous", 0, max_databases, &worker_rendezvous_ctl,
                    HASH_ELEM | HASH_BLOBS | HASH_ATTACH);
  LWLock *lock = &(GetNamedLWLockTranche("omni_ext_worker_rendezvous")->lock);

  // This stores locally initialized extensions
  cset_oid extensions = cset_oid_init();

  while (!terminate) {

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

      // With this extensions,
      // For every dynpgext handle registered in the startup phase
      c_FOREACH(handle, cdeq_handle, handles) {
        // Check if this handles matches the extension we're currently looking at
        char *cversion = text_to_cstring(version);
        bool matching = strcmp((*handle.ref)->name, ext->extname.data) == 0 &&
                        strcmp((*handle.ref)->version, cversion) == 0;
        pfree(cversion);
        // Ignore it if it's not matching
        if (!matching)
          continue;

        // If the worker already has already initialized this extension, skip
        if (!cset_oid_insert(&extensions, ext->oid).inserted)
          continue;

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
            // If this worker is not database-local (aka global), ensure we're the only ones
            // starting it
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
            bgw.ref->bgw.bgw_restart_time = BGW_NEVER_RESTART;
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
    if (scan->rs_rd->rd_tableam->scan_end) {
      scan->rs_rd->rd_tableam->scan_end(scan);
    }
    table_close(rel, AccessShareLock);

    // Ensure we're not getting a XID
    Assert(GetCurrentTransactionIdIfAny() == InvalidTransactionId);
    AbortCurrentTransaction();

    (void)WaitLatch(MyLatch, WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH, 1000L,
                    PG_WAIT_EXTENSION);
    ResetLatch(MyLatch);

    CHECK_FOR_INTERRUPTS();
  }

  cset_oid_drop(&extensions);
}