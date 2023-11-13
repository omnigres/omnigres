#include <dirent.h>
#include <limits.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <miscadmin.h>
#include <port.h>
#include <storage/fd.h>
#include <storage/ipc.h>
#include <storage/lwlock.h>
#include <storage/shmem.h>
#include <utils/dsa.h>
#include <utils/guc.h>
#include <utils/hsearch.h>

#include <libgluepg_stc.h>
#include <libgluepg_stc/str.h>
#include <libgluepg_stc/sview.h>

#include <libpgaug.h>

#include "control_file.h"
#include "omni_ext.h"
#include "workers.h"

#include <dynpgext.h>

PG_MODULE_MAGIC;

#if PG_MAJORVERSION_NUM >= 15
static shmem_request_hook_type saved_shmem_request_hook;
#endif

static shmem_startup_hook_type saved_shmem_startup_hook;

/**
 * @brief Requests required shared memory
 *
 */
void shmem_request() {
#if PG_MAJORVERSION_NUM >= 15
  if (saved_shmem_request_hook) {
    saved_shmem_request_hook();
  }
#endif

  // Allocate what Dynpgext extensions have requested
  c_FOREACH(req, cdeq_allocation_request, allocation_requests) {
    int multiplier = 1;
    if (req.ref->flags & DYNPGEXT_SCOPE_DATABASE_LOCAL) {
      multiplier = max_databases;
    }
    RequestAddinShmemSpace(req.ref->size * max_databases);
    RequestAddinShmemSpace(
        hash_estimate_size(max_allocation_dictionary_entries, sizeof(allocation_dictionary_entry)));
    RequestAddinShmemSpace(hash_estimate_size(max_databases, sizeof(database_oid_mapping_entry)));
  }

  // Allocate bulk of memory to be used for new allocations
  RequestAddinShmemSpace(shmem_size * 1024 * 1024);

  RequestNamedLWLockTranche("omni_ext_allocation_dictionary", 1);
  RequestNamedLWLockTranche("omni_ext_database_oid_mapping", 1);
  RequestNamedLWLockTranche("omni_ext_worker_rendezvous", 1);
}

/**
 * @brief Uses requested shared memory
 *
 */
void shmem_hook() {
  if (saved_shmem_startup_hook) {
    saved_shmem_startup_hook();
  }

  // Calculate if we have enough `max_worker_processes`
  bool found_bgworker_data = false;

#if PG_MAJORVERSION_NUM <= 16
  // This relies on internal knowledge of `BackgroundWorkerArray` struct.
  // Not perfect, but I haven't found a better way to get a number of used workers
  // at this stage
  struct _BackgroundWorkerSlot {
    bool in_use;
    bool terminate;
    pid_t pid;
    uint64 generation;
    BackgroundWorker worker;
  };
  struct _BackgroundWorkerArray {
    int total_slots;
    uint32 parallel_register_count;
    uint32 parallel_terminate_count;
    struct _BackgroundWorkerSlot slot[FLEXIBLE_ARRAY_MEMBER];
  };
  Size background_worker_shmem_size = offsetof(struct _BackgroundWorkerArray, slot);
  background_worker_shmem_size =
      add_size(background_worker_shmem_size,
               mul_size(max_worker_processes, sizeof(struct _BackgroundWorkerSlot)));
  struct _BackgroundWorkerArray
#else
#error                                                                                             \
    "Check if BackgroundWorkerArray/BackgroundWorkerSlot have changed in this version of Postgres"
#endif
      *worker_array = ShmemInitStruct("Background Worker Data", background_worker_shmem_size,
                                      &found_bgworker_data);

  int total_used = cdeq_background_worker_request_size(&background_worker_requests);
  for (int i = 0; i < worker_array->total_slots; i++) {
    if (worker_array->slot[i].in_use)
      total_used++;
  }

  if (max_worker_processes < total_used) {
    ereport(ERROR, errmsg("Not enough workers allowed. Consider changing max_workers_processes "
                          "from %d to at least %d",
                          max_parallel_workers, total_used));
  }

  LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);

  // Initialize DSA
  bool found_shmem = false;
  My_dsa_mem = ShmemInitStruct("omni_ext_shmem", shmem_size * 1024 * 1024, &found_shmem);
  int tranche_id = LWLockNewTrancheId();
  My_dsa_area = dsa_create_in_place(My_dsa_mem, shmem_size * 1024 * 1024, tranche_id, NULL);
  My_dsa_pid = MyProcPid;

  {
    // Initialize the database oid mapping dictionary
    HTAB *dict = ShmemInitHash("omni_ext_database_oid_mapping", 0, max_databases,
                               &database_oid_mapping_ctl, HASH_ELEM | HASH_BLOBS);
  }

  {
    // Initialize the allocation dictionary
    HTAB *dict = ShmemInitHash(
        "omni_ext_allocation_dictionary", cdeq_allocation_request_size(&allocation_requests),
        max_allocation_dictionary_entries, &allocation_dictionary_ctl, HASH_ELEM | HASH_STRINGS);

    LWLock *lock = &(GetNamedLWLockTranche("omni_ext_allocation_dictionary")->lock);

    c_FOREACH(req, cdeq_allocation_request, allocation_requests) {
      allocation_request *request = req.ref;
      int flags = 0;
      int multiplier = 1;
      if ((req.ref->flags & DYNPGEXT_SCOPE_DATABASE_LOCAL) == DYNPGEXT_SCOPE_DATABASE_LOCAL) {
        multiplier = max_databases;
      }
      size_t size = request->size * multiplier;
      if (size >= 1 * 1024 * 1024 * 1024) {
        flags = DSA_ALLOC_HUGE;
      }
      dsa_pointer ptr = dsa_allocate_extended(My_dsa_area, size, flags);
      LWLockAcquire(lock, LW_EXCLUSIVE);
      bool found = false;
      allocation_dictionary_entry *entry =
          (allocation_dictionary_entry *)hash_search(dict, request->name, HASH_ENTER, &found);
      if (!found) {
        entry->ptr = ptr;
        entry->flags = request->flags;
        entry->size = request->size;
      }
      LWLockRelease(lock);

      if (request->callback) {
        void *addr = dsa_get_address(My_dsa_area, ptr);
        for (int i = 0; i < multiplier; i++) {
          request->callback(addr + (i * request->size), request->data);
        }
      }
    }
  }

  {
    HTAB *dict = ShmemInitHash("omni_ext_worker_rendezvous", 0, max_databases,
                               &worker_rendezvous_ctl, HASH_ELEM | HASH_BLOBS);
  }

  LWLockRelease(AddinShmemInitLock);
}

static bool _dynpgext_loader_present = true;

void _PG_init() {
  DefineCustomBoolVariable("dynpgext.loader_present",
                           "Flag indicating presence of a Dynpgext loader", NULL,
                           &_dynpgext_loader_present, true, PGC_BACKEND, 0, NULL, NULL, NULL);

  DefineCustomIntVariable("omni_ext.shmem_size",
                          "Pre-allocated shared memory size, rounded to megabytes", NULL,
                          &shmem_size, 16, 0, MAX_KILOBYTES, PGC_POSTMASTER,
                          GUC_UNIT_MB
#if PG_MAJORVERSION_NUM >= 15
                              | GUC_RUNTIME_COMPUTED
#endif
                          ,
                          NULL, NULL, NULL);

  DefineCustomIntVariable("omni_ext.max_databases",
                          "Maximum number of databases to be used in database-local allocations",
                          NULL, &max_databases, 32, 0, INT_MAX, PGC_POSTMASTER, 0, NULL, NULL,
                          NULL);

  DefineCustomIntVariable("omni_ext.max_allocation_dictionary_entries",
                          "Maximum number of allocation dictionary entries", NULL,
                          &max_allocation_dictionary_entries, 1000, 0, INT_MAX, PGC_POSTMASTER, 0,
                          NULL, NULL, NULL);
  MemoryContext old_context = MemoryContextSwitchTo(TopMemoryContext);

  // Initialize storage for requests
  allocation_requests = cdeq_allocation_request_init();
  background_worker_requests = cdeq_background_worker_request_init();
  handles = cdeq_handle_init();

  // Scan the directory unless disabled
  if (getenv("OMNI_EXT_NOPRELOAD") == NULL) {
    struct load_control_file_config config = {.preload = true,
                                              .allocate_shmem = allocate_shmem_startup,
                                              .expected_version = NULL,
                                              .register_bgworker_function =
                                                  register_bgworker_startup};
    find_control_files(load_control_file, (void *)&config);

    // Initialize
#if PG_MAJORVERSION_NUM >= 15
    saved_shmem_request_hook = shmem_request_hook;
    shmem_request_hook = &shmem_request;
#else
    shmem_request();
#endif

    saved_shmem_startup_hook = shmem_startup_hook;
    shmem_startup_hook = shmem_hook;
  }

  BackgroundWorker master_worker = {.bgw_name = "omni_ext master",
                                    .bgw_type = "omni_ext master",
                                    .bgw_flags = BGWORKER_SHMEM_ACCESS |
                                                 BGWORKER_BACKEND_DATABASE_CONNECTION,
                                    .bgw_start_time = BgWorkerStart_RecoveryFinished,
                                    .bgw_restart_time = 0,
                                    .bgw_function_name = "master_worker",
                                    .bgw_notify_pid = 0};
  strncpy(master_worker.bgw_library_name, get_library_name(), BGW_MAXLEN);
  RegisterBackgroundWorker(&master_worker);
  MemoryContextSwitchTo(old_context);
}
