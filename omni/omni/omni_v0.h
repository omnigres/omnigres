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
#include <storage/lwlock.h>
#include <tcop/utility.h>
#include <utils/guc.h>
#include <utils/guc_tables.h>

#if PG_MAJORVERSION_NUM >= 13 && PG_MAJORVERSION_NUM <= 16
/**
 * Omni's copy of an otherwise private datatype of `BackgroundWorkerHnalde`
 * so that it can be copied between backends.
 */
typedef struct BackgroundWorkerHandle {
  int slot;
  uint64 generation;
} BackgroundWorkerHandle;

#else
#error "Ensure this version of Postgres is compatible with the above definition or use a new one"
#endif

/**
 * @private
 * @brief Magic structure for compatibility checks
 */
typedef struct {
  uint16_t size;    // size of this structure
  uint16_t version; // interface version
  uint8_t revision; // version's backward compatible revision (number, but shown as letter 'A','B'
                    // to distinguish for Major.Minor. Version 0 allows for breaking revisions.
} omni_magic;

StaticAssertDecl(sizeof(omni_magic) <= UINT16_MAX, "omni_magic should fit into 16 bits");

#define OMNI_INTERFACE_VERSION 0
#define OMNI_INTERFACE_REVISION 6

typedef struct omni_handle omni_handle;

/**
 * @brief Extension initialization callback, called once per backend
 *
 * Defined by the extension. Optional but highly recommended.
 *
 * Will be called in a memory context that is specific to this module.
 *
 * @param handle Loader handle
 */
void _Omni_init(const omni_handle *handle);

/**
 * @brief Extension de-initialization callback, called once per backend
 *
 * Defined by the extension. Optional but highly recommended.
 *
 * Will be called in a memory context that is specific to this module.
 *
 * @param handle Loader handle
 */
void _Omni_deinit(const omni_handle *handle);

/**
 * @brief Every dynamic extension should use this macro in addition to `PG_MODULE_MAGIC`
 *
 */
#define OMNI_MAGIC                                                                                 \
  static omni_magic __Omni_magic = {.size = sizeof(omni_magic),                                    \
                                    .version = OMNI_INTERFACE_VERSION,                             \
                                    .revision = OMNI_INTERFACE_REVISION};                          \
  omni_magic *_Omni_magic() { return &__Omni_magic; }

/**
 * @brief Shared memory allocation callback
 *
 * @param handle Handle passed by the loader
 * @param ptr Allocation
 * @param data Extra parameter passed through `allocate_shmem`
 * @param allocated True when the area was allocated in this call, false if it was found
 */
typedef void (*omni_allocate_shmem_callback_function)(const omni_handle *handle, void *ptr,
                                                      void *data, bool allocated);
/**
 * @brief Shared memory allocation function
 *
 * @param handle Handle passed by the loader
 * @param name Name to register this allocation under. It is advised to include
 *             version information into the name to facilitate easier upgrades
 * @param size Amount of memory to allocate
 * @param init Callback to initialize allocate memory. Can be `NULL`
 * @param data Extra parameter to pass to `init`
 * @param found Pointer to a flag indicating if the allocation was found
 *
 */
typedef void *(*omni_allocate_shmem_function)(const omni_handle *handle, const char *name,
                                              size_t size,
                                              omni_allocate_shmem_callback_function init,
                                              void *data, bool *found);

/**
 * @brief Shared memory deallocation function
 *
 * @param handle Handle passed by the loader
 * @param name Name this allocation was registered under
 * @param found Pointer to a flag indicating if the allocation was found
 */
typedef void (*omni_deallocate_shmem_function)(const omni_handle *handle, const char *name,
                                               bool *found);

/**
 * @brief Function to lookup previously allocated shared memory
 *
 * This function is defined by the loader.
 *
 * @param handle handle
 * @param name name it was registered under
 * @param found indicates if it was found
 * @return void* pointer to the allocation, NULL if none found
 */
typedef void *(*omni_lookup_shmem_function)(const omni_handle *handle, const char *name,
                                            bool *found);

/**
 * @brief Register a lock under a given name
 *
 * It is guaranteed to have the same under every backend.
 *
 * @param handle Handle passed by the loader
 * @param name Name of the lock. Must be static or allocated under a top memory context
 */
typedef void (*omni_register_lwlock_function)(const omni_handle *handle, LWLock *lock,
                                              const char *name, bool initialize);

/**
 * @brief Unregister a lock
 *
 * This will allow to reclaim tranche ID for future use (subject to loader's support and algorithm)
 *
 * Would be typically called from `_Omni_unload` to ensure it is called once, when all backends
 * have deinitialized.
 */
typedef void (*omni_unregister_lwlock_function)(const omni_handle *handle, LWLock *lock);

typedef struct omni_handle omni_handle;

typedef enum { hook_next_action_finish = 0, hook_next_action_next = 1 } hook_next_action;
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
  omni_hook_emit_log = 0,
  omni_hook_check_password = 1,
  omni_hook_client_authentication = 2,
  omni_hook_executor_check_perms = 3,
  omni_hook_object_access = 3,
  omni_hook_row_security_policy_permissive = 4,
  omni_hook_row_security_policy_restrictive = 5,
  omni_hook_needs_fmgr = 6,
  omni_hook_fmgr = 7,
  omni_hook_explain_get_index_name = 8,
  omni_hook_explain_one_query = 9,
  omni_hook_get_attavg_width = 10,
  omni_hook_get_index_stats = 11,
  omni_hook_get_relation_info = 12,
  omni_hook_get_relation_stats = 13,
  omni_hook_planner = 14,
  omni_hook_join_search = 15,
  omni_hook_set_rel_pathlist = 16,
  omni_hook_set_join_pathlist = 17,
  omni_hook_create_upper_paths = 18,
  omni_hook_post_parse_analyze = 19,
  omni_hook_executor_start = 20,
  omni_hook_executor_run = 21,
  omni_hook_executor_finish = 22,
  omni_hook_executor_end = 23,
  omni_hook_process_utility = 24,
  omni_hook_xact_callback = 25,
  omni_hook_subxact_callback = 26,
  omni_hook_plpgsql_func_setup = 27,
  omni_hook_plpgsql_func_beg = 28,
  omni_hook_plpgsql_func_end = 29,
  omni_hook_plpgsql_stmt_beg = 30,
  omni_hook_plpgsql_stmt_end = 31,
  __OMNI_HOOK_TYPE_COUNT // Counter
} omni_hook_type;

typedef enum {
  omni_hook_position_default = 0,
  omni_hook_position_leading = 1 << 0,
  omni_hook_position_trailing = 1 << 1
} omni_hook_position;

typedef struct {
  omni_hook_type type;
  omni_hook_fn fn;
  char *name;
  omni_hook_position position;
} omni_hook;

/**
 * @brief Function to register a hook in a backend
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
 * @param hook Hook to register. `hook` doesn't need to be allocated in a memory context, but
 * `hook->name` must be allocated either statically or in a context that outlives the module (either
 * the module's own memory context, or something that would live longer than that)
 */
typedef void (*omni_register_hook_function)(const omni_handle *handle, omni_hook *hook);

typedef struct {
  int *value;

  GucIntCheckHook check_hook;
  GucIntAssignHook assign_hook;

  int boot_value;
  int min_value;
  int max_value;
} omni_guc_int_variable;

StaticAssertDecl(offsetof(omni_guc_int_variable, value) == 0,
                 "to ensure casting to (int *) will get us straight to the pointer");

typedef struct {
  bool *value;

  GucBoolCheckHook check_hook;
  GucBoolAssignHook assign_hook;

  bool boot_value;
} omni_guc_bool_variable;

StaticAssertDecl(offsetof(omni_guc_bool_variable, value) == 0,
                 "to ensure casting to (bool *) will get us straight to the pointer");

typedef struct {
  double *value;

  GucRealCheckHook check_hook;
  GucRealAssignHook assign_hook;

  double boot_value;
  double min_value;
  double max_value;
} omni_guc_real_variable;

StaticAssertDecl(offsetof(omni_guc_real_variable, value) == 0,
                 "to ensure casting to (double *) will get us straight to the pointer");

typedef struct {
  char **value;

  GucStringCheckHook check_hook;
  GucStringAssignHook assign_hook;

  char *boot_value;
} omni_guc_string_variable;

StaticAssertDecl(offsetof(omni_guc_string_variable, value) == 0,
                 "to ensure casting to (char *) will get us straight to the pointer");

typedef struct {
  int *value;

  GucIntCheckHook check_hook;
  GucIntAssignHook assign_hook;

  int boot_value;

  const struct config_enum_entry *options;
} omni_guc_enum_variable;

StaticAssertDecl(offsetof(omni_guc_enum_variable, value) == 0,
                 "to ensure casting to (int *) will get us straight to the pointer");

typedef struct {
  const char *name;
  const char *short_desc;
  const char *long_desc;
  enum config_type type;
  union {
    omni_guc_bool_variable bool_val;
    omni_guc_int_variable int_val;
    omni_guc_real_variable real_val;
    omni_guc_string_variable string_val;
    omni_guc_enum_variable enum_val;
  } typed;
  GucContext context;
  int flags;
  GucShowHook show_hook;
} omni_guc_variable;

typedef void (*omni_declare_guc_variable_function)(const omni_handle *handle,
                                                   omni_guc_variable *variable);

typedef enum {
  omni_timing_after_commit = 0,
  omni_timing_at_commit = 1,
  omni_timing_immediately = 2
} omni_timing;

typedef struct {
  omni_timing timing;
  bool dont_wait;
} omni_bgworker_options;

typedef struct omni_bgworker_handle {
  BackgroundWorkerHandle bgw_handle;
  bool registered;
} omni_bgworker_handle;

typedef void (*omni_request_bgworker_start_function)(const omni_handle *handle,
                                                     BackgroundWorker *bgworker,
                                                     omni_bgworker_handle *bgworker_handle,
                                                     const omni_bgworker_options options);

typedef void (*omni_request_bgworker_termination_function)(const omni_handle *handle,
                                                           omni_bgworker_handle *bgworker_handle,
                                                           const omni_bgworker_options options);

/**
 * Switch operation
 */
typedef enum { omni_switch_off = 0, omni_switch_on = 1 } omni_switch_operation;

/**
 * @brief Function that flips switches to desired state
 *
 * Switches are simple 1/0 values that can be atomically switched either to `on` or `off` state.
 * This function returns a number that sets bits that were flipped by this operation to `1`. This
 * allows the caller to be able to determine if they were the first party to flip the switch,
 * enabling a primitive form of leader election.
 *
 * @param handle
 * @param op `omni_switch_on` turns the switches on, `omni_switch_off` turns the switches off
 * @param switchboard switchboard ID
 *        Omni provides 64 switches per switchboard. At least one switchboard must be provided
 *        (`switchboard = 0`) by the loader, attempts to get `switchboard` greater than zero,
 *        may trigger `ERROR` if no more switchboards can be allocated.
 * @param mask select switches to perform `op` on
 *
 */
typedef uint64 (*omni_atomic_switch_function)(const omni_handle *handle, omni_switch_operation op,
                                              uint32 switchboard, uint64 mask);

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

  omni_allocate_shmem_function allocate_shmem;
  omni_deallocate_shmem_function deallocate_shmem;
  omni_lookup_shmem_function lookup_shmem;

  omni_register_hook_function register_hook;

  omni_declare_guc_variable_function declare_guc_variable;

  omni_request_bgworker_start_function request_bgworker_start;
  omni_request_bgworker_termination_function request_bgworker_termination;

  omni_register_lwlock_function register_lwlock;
  omni_unregister_lwlock_function unregister_lwlock;

  omni_atomic_switch_function atomic_switch;

} omni_handle;

#endif // OMNI_H