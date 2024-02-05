/*! \file */
#ifndef OMNI_H
#define OMNI_H

#include <dlfcn.h>
#include <stddef.h>

// clang-format off
#include <postgres.h>
#include <postmaster/bgworker.h>
// clang-format on
#include <access/xact.h>
#include <executor/executor.h>
#include <libpq/crypt.h>
#include <nodes/pathnodes.h>
#include <nodes/plannodes.h>
#include <tcop/utility.h>
#include <utils/guc.h>

/**
 * @private
 * @brief Magic structure for compatibility checks
 */
typedef struct {
  size_t size;
  uint8_t version;
} omni_magic;

typedef struct omni_handle omni_handle;

/**
 * @brief Extension initialization callback, called once per load
 *
 * Defined by the extension. Optional but highly recommended.
 *
 * @param handle Loader handle
 */
void _Omni_load(const omni_handle *handle);

/**
 * @brief Extension initialization callback, called once per backend
 *
 * Defined by the extension. Optional but highly recommended.
 *
 * @param handle Loader handle
 */
void _Omni_init(const omni_handle *handle);

/**
 * @brief Extension de-initialization callback, called once per backend
 *
 * Defined by the extension. Optional but highly recommended.
 *
 * @param handle Loader handle
 */
void _Omni_deinit(const omni_handle *handle);

/**
 * @brief Extension deinitialization callback, on unload
 *
 * It cannot be guaranteed to be called in any specific order in relation to to `_Omni_deinit`
 * but is guaranteed to be called once (before another load may take place)
 *
 * Defined by the extension. Optional.
 *
 * @param handle Loader handle
 */
void _Omni_unload(const omni_handle *handle);

/**
 * @brief Every dynamic extension should use this macro in addition to `PG_MODULE_MAGIC`
 *
 */
#define OMNI_MAGIC                                                                                 \
  static omni_magic __Omni_magic = {.size = sizeof(omni_magic), .version = 0};                     \
  omni_magic *_Omni_magic() { return &__Omni_magic; }

/**
 * @brief Shared memory allocation function
 *
 * @param handle Handle passed by the loader
 * @param name Name to register this allocation under. It is advised to include
 *             version information into the name to facilitate easier upgrades
 * @param size Amount of memory to allocate
 * @param found Pointer to a flag indicating if the allocation was found
 *
 */
typedef void *(*omni_allocate_shmem_function)(const omni_handle *handle, const char *name,
                                              size_t size, bool *found);

/**
 * @brief Shared memory deallocation functon
 * @param handle Handle passed by the loader
 * @param name Name this allocation was registered under
 * @param found Pointer to a flag indicating if the allocation was found
 */
typedef void (*omni_deallocate_shmem_function)(const omni_handle *handle, const char *name,
                                               bool *found);

typedef struct omni_handle omni_handle;

typedef enum { finish, next } hook_next_action;
typedef union {
  bool bool_value;
  char *char_value;
  List *list_value;
  int32 int32_value;
  PlannedStmt *PlannedStmt_value;
  RelOptInfo *RelOptInfo_value;
  void *ptr_value;
} omni_hook_return_value;

typedef struct {
  const omni_handle *handle;
  void *ctx;
  hook_next_action next_action;
  omni_hook_return_value returns;
} omni_hook_handle;

typedef void (*omni_hook_emit_log_t)(omni_hook_handle *handle, ErrorData *edata);
typedef void (*omni_hook_check_password_t)(omni_hook_handle *handle, const char *username,
                                           const char *shadow_pass, PasswordType password_type,
                                           Datum validuntil_time, bool validuntil_null);

typedef void (*omni_hook_needs_fmgr_t)(omni_hook_handle *handle, Oid fn_oid);
typedef void (*omni_hook_executor_start_t)(omni_hook_handle *handle, QueryDesc *queryDesc,
                                           int eflags);
typedef void (*omni_hook_executor_run_t)(omni_hook_handle *handle, QueryDesc *queryDesc,
                                         ScanDirection direction, uint64 count, bool execute_once);
typedef void (*omni_hook_executor_finish_t)(omni_hook_handle *handle, QueryDesc *queryDesc);
typedef void (*omni_hook_executor_end_t)(omni_hook_handle *handle, QueryDesc *queryDesc);
typedef void (*omni_hook_process_utility_t)(omni_hook_handle *handle, PlannedStmt *pstmt,
                                            const char *queryString, bool readOnlyTree,
                                            ProcessUtilityContext context, ParamListInfo params,
                                            QueryEnvironment *queryEnv, DestReceiver *dest,
                                            QueryCompletion *qc);
typedef void (*omni_hook_xact_callback_t)(omni_hook_handle *handle, XactEvent event);
typedef void (*omni_hook_subxact_callback_t)(omni_hook_handle *handle, SubXactEvent event);

typedef union {
  omni_hook_emit_log_t emit_log;
  omni_hook_check_password_t check_password;
  omni_hook_needs_fmgr_t needs_fmgr;
  omni_hook_executor_start_t executor_start;
  omni_hook_executor_run_t executor_run;
  omni_hook_executor_finish_t executor_finish;
  omni_hook_executor_end_t executor_end;
  omni_hook_process_utility_t process_utility;
  omni_hook_xact_callback_t xact_callback;
  omni_hook_subxact_callback_t subxact_callback;
  void *ptr;
} omni_hook_fn;

// Assigned values MUST be consequtive, linear and start from 0 for __OMNI_HOOK_TYPE_COUNT to be
// correct
typedef enum {
  omni_hook_emit_log,
  omni_hook_check_password,
  omni_hook_client_authentication,
  omni_hook_executor_check_perms,
  omni_hook_object_access,
  omni_hook_row_security_policy_permissive,
  omni_hook_row_security_policy_restrictive,
  omni_hook_needs_fmgr,
  omni_hook_fmgr,
  omni_hook_explain_get_index_name,
  omni_hook_explain_one_query,
  omni_hook_get_attavg_width,
  omni_hook_get_index_stats,
  omni_hook_get_relation_info,
  omni_hook_get_relation_stats,
  omni_hook_planner,
  omni_hook_join_search,
  omni_hook_set_rel_pathlist,
  omni_hook_set_join_pathlist,
  omni_hook_create_upper_paths,
  omni_hook_post_parse_analyze,
  omni_hook_executor_start,
  omni_hook_executor_run,
  omni_hook_executor_finish,
  omni_hook_executor_end,
  omni_hook_process_utility,
  omni_hook_xact_callback,
  omni_hook_subxact_callback,
  omni_hook_plpgsql_func_setup,
  omni_hook_plpgsql_func_beg,
  omni_hook_plpgsql_func_end,
  omni_hook_plpgsql_stmt_beg,
  omni_hook_plpgsql_stmt_end,
  __OMNI_HOOK_TYPE_COUNT // Counter
} omni_hook_type;

typedef struct {
  omni_hook_type type;
  omni_hook_fn fn;
  char *name;
  bool wrap : 1;
} omni_hook;

/**
 * @brief Handle provided by the loader
 *
 */
typedef struct omni_handle {
  /**
   * @brief Shared library (.so) name
   *
   * Can be used to register background worker from the same library.
   *
   */
  char *(*get_library_name)(const omni_handle *handle);
  /**
   * @brief Shared memory allocation function
   *
   */
  omni_allocate_shmem_function allocate_shmem;

  /**
   * @brief Shared memory deallocation function
   */
  omni_deallocate_shmem_function deallocate_shmem;

  /**
   * @brief Register a hook in a backend
   *
   * Best place to register the hook is `_Omni_init`, they hook will be registered in every backend.
   *
   * If done in `_Omni_load`, it will be only registered in the backend that caused the module to be
   * loaded.
   *
   * If done during a regular function call, the backend will be only registered in the backend
   * where it was called.
   *
   * @param handle
   * @param hook
   */
  void (*register_hook)(const omni_handle *handle, omni_hook *hook);

  /**
   * @brief Lookup previously allocated shared memory
   *
   * This function is defined by the loader.
   *
   * @param handle handle
   * @param name name it was registered under
   * @param found indicates if it was found
   * @return void* pointer to the allocation, NULL if none found
   */
  void *(*lookup_shmem)(const omni_handle *handle, const char *name, bool *found);

} omni_handle;

#if PG_MAJORVERSION_NUM >= 13 && PG_MAJORVERSION_NUM <= 16
/**
 * Omni's copy of an otherwise private datatype of `BackgroundWorkerHnalde`
 * so that it can be copied between backends.
 */
typedef struct {
  int slot;
  uint64 generation;
} OmniBackgroundWorkerHandle;

/**
 * Wrapper for `RegisterDynamicBackgroundWorker` to get `OmniBackgroundWorkerHandle`
 * instead of dynamically allocated `BackgroundWorkerHandle`.
 *
 * @param worker background worker definition
 * @param handle must be a non-NULL pointer to `OmniBackgroundWorkerHandle`
 */
static inline bool OmniRegisterDynamicBackgroundWorker(BackgroundWorker *worker,
                                                       OmniBackgroundWorkerHandle *handle) {
  BackgroundWorkerHandle *bgw_handle;
  bool result = RegisterDynamicBackgroundWorker(worker, &bgw_handle);
  memcpy(handle, bgw_handle, sizeof(OmniBackgroundWorkerHandle));
  // we must free `bgw_handle` because the memory context we're in may be long-living
  pfree(bgw_handle);
  return result;
}

/**
 * @brief Convert `OmniBackgroundWorkerHandle` to `BackgroundWorkerHandle`
 */
static inline BackgroundWorkerHandle *
GetBackgroundWorkerHandle(OmniBackgroundWorkerHandle *handle) {
  return (BackgroundWorkerHandle *)handle;
}

#else
#error "Ensure this version of Postgres is compatible with the above definition or use a new one"
#endif

#endif // OMNI_H