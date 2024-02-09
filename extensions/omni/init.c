// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/heapam.h>
#include <commands/user.h>
#include <executor/executor.h>
#include <lib/dshash.h>
#include <miscadmin.h>
#include <storage/ipc.h>
#include <storage/lwlock.h>
#include <storage/shmem.h>
#include <utils/hsearch.h>
#include <utils/inval.h>
#include <utils/memutils.h>
#include <utils/snapmgr.h>

#include "omni_common.h"

#if PG_MAJORVERSION_NUM >= 15
// Previous shmem_request hook
static shmem_request_hook_type saved_shmem_request_hook;
#endif

// Previous shmem_startup hook
static shmem_startup_hook_type saved_shmem_startup_hook;

static void shmem_request();
static void shmem_hook();

void procoid_syscache_callback(Datum arg, int cacheid, uint32 hashvalue) {
  backend_force_reload = true;
}
/**
 * Shared preload initialization.
 */
void _PG_init() {
  memset(saved_hooks, 0, sizeof(saved_hooks));
  // This signifies if this library has indeed been preloaded
  static bool preloaded = false;
  // We only initialize once, as a shared preloaded library.
  if (!process_shared_preload_libraries_in_progress) {
    if (!preloaded) {
      // Issue a warning if it is not preloaded, as it won't be useful
      ereport(WARNING, errmsg("omni extension has not been preloaded"),
              errhint("`shared_preload_libraries` should list `omni`"));
    }
    // If it is not being preloaded, nothing to do here
    return;
  }

  preloaded = true;

  // Prepare shared memory
#if PG_MAJORVERSION_NUM >= 15
  saved_shmem_request_hook = shmem_request_hook;
  shmem_request_hook = &shmem_request;
#else
  shmem_request();
#endif

  saved_shmem_startup_hook = shmem_startup_hook;
  shmem_startup_hook = shmem_hook;

  // Here we save all the conventional hooks we support

#define save_hook(NAME, OLD)                                                                       \
  saved_hooks[omni_hook_##NAME] = OLD;                                                             \
  OLD = omni_##NAME##_hook

  save_hook(needs_fmgr, needs_fmgr_hook);
  save_hook(executor_start, ExecutorStart_hook);
  save_hook(executor_run, ExecutorRun_hook);
  save_hook(executor_finish, ExecutorFinish_hook);
  save_hook(executor_end, ExecutorEnd_hook);
  save_hook(process_utility, ProcessUtility_hook);
  save_hook(emit_log, emit_log_hook);
  save_hook(check_password, check_password_hook);
#undef save_hook

  RegisterXactCallback(omni_xact_callback_hook, NULL);

  void *default_hooks[__OMNI_HOOK_TYPE_COUNT] = {
      [omni_hook_needs_fmgr] = saved_hooks[omni_hook_needs_fmgr] ? default_needs_fmgr : NULL,
      [omni_hook_executor_start] = default_executor_start,
      [omni_hook_executor_run] = default_executor_run,
      [omni_hook_executor_finish] = default_executor_finish,
      [omni_hook_executor_end] = default_executor_end,
      [omni_hook_process_utility] = default_process_utility,
      [omni_hook_emit_log] = saved_hooks[omni_hook_emit_log] ? default_emit_log : NULL,
      [omni_hook_check_password] =
          saved_hooks[omni_hook_check_password] ? default_check_password_hook : NULL,
      NULL};

  {
    // These entrypoints will be initialized and copies to forked processes (backends)
    // Let's ensure we're in the TopMemoryContext when we allocate these
    MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
    for (int type = 0; type < __OMNI_HOOK_TYPE_COUNT; type++) {
      // It is important to use palloc0 here to ensure the first recors in the hook array are
      // calling the original hooks (if present)
      if (default_hooks[type] != NULL) {
        hook_entry_point *entry;
        entry = hook_entry_points.entry_points[type] =
            palloc0(sizeof(*hook_entry_points.entry_points[type]));
        hook_entry_points.entry_points_count[type] = 1;
        entry->fn = default_hooks[type];
        entry->name = "default";
      }
    }
    MemoryContextSwitchTo(oldcontext);
  }

  {
    // Initialize the backend when PostmasterContext is deleted
    MemoryContext oldcontext = MemoryContextSwitchTo(PostmasterContext);
    MemoryContextCallback *callback = palloc(sizeof(*callback));
    callback->func = init_backend;
    MemoryContextRegisterResetCallback(PostmasterContext, callback);
    MemoryContextSwitchTo(oldcontext);
  }

  {
    BackgroundWorker master_worker = {.bgw_name = "omni startup",
                                      .bgw_type = "omni startup",
                                      .bgw_flags = BGWORKER_SHMEM_ACCESS |
                                                   BGWORKER_BACKEND_DATABASE_CONNECTION,
                                      .bgw_start_time = BgWorkerStart_RecoveryFinished,
                                      .bgw_restart_time = BGW_NEVER_RESTART,
                                      .bgw_function_name = "startup_worker",
                                      .bgw_notify_pid = 0};
    strncpy(master_worker.bgw_library_name, get_omni_library_name(), BGW_MAXLEN);
    RegisterBackgroundWorker(&master_worker);
  }

  // This function may result in a FATAL error if we hit the limit of
  // syscache callbacks (`MAX_SYSCACHE_CALLBACKS`, 64). However, since we're in
  // postmaster: 1) it is okay to crash here 2) it is much less likely to happen at this time.
  CacheRegisterSyscacheCallback(PROCOID, procoid_syscache_callback, Int8GetDatum(0));
  backend_force_reload = true;

  OmniGUCContext = AllocSetContextCreate(TopMemoryContext, "omni:guc", ALLOCSET_DEFAULT_SIZES);

  omni_modules = NULL;
}

/**
 * @brief Requests required shared memory
 *
 */
static void shmem_request() {
#if PG_MAJORVERSION_NUM >= 15
  if (saved_shmem_request_hook) {
    saved_shmem_request_hook();
  }
#endif

  RequestAddinShmemSpace(sizeof(omni_shared_info));

  RequestNamedLWLockTranche("omni", __omni_num_locks);
}

/**
 * @brief Uses requested shared memory
 *
 */
static void shmem_hook() {
  if (saved_shmem_startup_hook) {
    saved_shmem_startup_hook();
  }

  LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);

  {
    bool found;
    shared_info = ShmemInitStruct("omni:shared_info", sizeof(omni_shared_info), &found);
    pg_atomic_write_u32(&shared_info->module_counter, 0);
    shared_info->dsa = 0;
    shared_info->modules_tab = InvalidDsaPointer;
    shared_info->allocations_tab = InvalidDsaPointer;
    pg_atomic_init_flag(&shared_info->tables_initialized);
  }

  LWLockRelease(AddinShmemInitLock);
  OMNI_DSA_TRANCHE = LWLockNewTrancheId();
}

/**
 * This function is used to "wait" until background worker has a database chosen
 *
 * Based on the internal knowledge of `InitPostgres` calling `CommitTransactionCommand`
 *
 * @param event
 * @param arg
 */
static void bgw_first_xact(XactEvent event, void *arg) {
#if PG_MAJORVERSION_NUM < 16
  // As you can see below, only Postgres 16 and onwards allow to deregister the callback
  // so for earlier versions, we simply silence it
  static bool done = false;
  if (done) {
    return;
  }
#endif

  if (event == XACT_EVENT_PRE_COMMIT) {
    // We capture at the pre-commit stage as this is where we're still in transaction state
    Assert(IsTransactionState());
    if (MyDatabaseId != InvalidOid) {
      init_backend(NULL);
#if PG_MAJORVERSION_NUM >= 16
      // Only unregister in Postgres >= 16 as per
      // https://github.com/postgres/postgres/commit/4d2a844242dcfb34e05dd0d880b1a283a514b16b In all
      // other versions, this callback will stay with its minimal (however, non-zero) cost.
      UnregisterXactCallback(bgw_first_xact, NULL);
#else
      done = true;
#endif
    }
  }
}

MODULE_FUNCTION void init_backend(void *arg) {
  if (MyBackendType == B_BACKEND || MyBackendType == B_BG_WORKER || MyBgworkerEntry != NULL) {
    if (MyBgworkerEntry != NULL) {
      if (strcmp(MyBgworkerEntry->bgw_library_name, "postgres") == 0) {
        // Don't do anything for `postgres` own workers
        return;
      }
      if (MyBackendType != B_BG_WORKER) {
        // It is too early for bgworkers to initialize (locks not available, etc.)
        //
        // So we wait for the first transaction with a database set to occur, to get back here
        RegisterXactCallback(bgw_first_xact, NULL);

        // Stop this initialization from proceeding any further for the time being
        // We will get back here through the trampoline described above
        return;
      }
      // We are back now
    } else {
      // We only open a transaction if it is a backend. Background worker is already
      // in a transaction (pre-commit).
      SetCurrentStatementStartTimestamp();
      StartTransactionCommand();
    }

    PushActiveSnapshot(GetTransactionSnapshot());

    ensure_backend_initialized();
    load_pending_modules();

    PopActiveSnapshot();

    if (MyBackendType != B_BG_WORKER) {
      // We only abort a transaction if it is a backend. Background worker is already
      // in a transaction (pre-commit).
      AbortCurrentTransaction();
    }
  }
}
