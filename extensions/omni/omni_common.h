#ifndef OMNI_COMMON_H
#define OMNI_COMMON_H
// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/table.h>
#include <catalog/pg_language.h>
#include <catalog/pg_proc.h>
#include <common/hashfn.h>
#include <executor/executor.h>
#include <lib/dshash.h>
#include <miscadmin.h>
#include <storage/ipc.h>
#include <storage/lwlock.h>
#include <storage/shmem.h>
#include <utils/builtins.h>
#include <utils/hsearch.h>
#include <utils/memutils.h>
#include <utils/rel.h>
#include <utils/syscache.h>
#if PG_MAJORVERSION_NUM < 15
#include "dshash.h"
#endif

#include <omni/omni_v0.h>

// If we are doing a unity build, module variables and functions
// are static. Otherwise, variable declarations are extern and variables
// and functions aren't static
#ifdef UNITY_BUILD
#define DECLARE_MODULE_VARIABLE(X) static X
#define MODULE_VARIABLE(X)
#define MODULE_FUNCTION static
#else
#define DECLARE_MODULE_VARIABLE(X) extern X
#define MODULE_VARIABLE(X) X
#define MODULE_FUNCTION
#endif

typedef struct {
  volatile pg_atomic_uint32 module_counter;
  pg_atomic_flag tables_initialized;
  dsa_handle dsa;
  dshash_table_handle modules_tab;
  dshash_table_handle allocations_tab;
} omni_shared_info;

extern omni_shared_info *shared_info;

typedef enum {
  OMNI_LOCK_MODULE,
  OMNI_LOCK_ALLOCATION,
  __omni_num_locks,
} omni_locks;

DECLARE_MODULE_VARIABLE(dshash_table *omni_modules);
DECLARE_MODULE_VARIABLE(dshash_table *omni_allocations);
DECLARE_MODULE_VARIABLE(HTAB *dsa_handles);

typedef enum { HANDLE_LOADED = 0, HANDLE_UNLOADED = 1 } omni_handle_state;

typedef struct {
  /**
   * Handle to share with the module
   */
  omni_handle handle;
  /**
   * Magic
   */
  omni_magic magic;
  /**
   * Unique handle ID
   */
  uint32 id;
  /**
   * Path the module
   */
  char path[PATH_MAX];
  /**
   * Reference counter
   *
   * @deprecated as of 0F
   */
  volatile pg_atomic_uint32 refcount;
  /**
   * Loaded flag
   */
  volatile pg_atomic_uint32 loaded;
  /**
   * DSA
   */
  dsa_handle dsa;
  /**
   *
   */
  pg_atomic_uint64 switchboard;
} omni_handle_private;

DECLARE_MODULE_VARIABLE(omni_handle_private *module_handles);

typedef struct {
  /**
   * Path as a key
   *
   * Used to locate it when observing loading a module.
   */
  char path[PATH_MAX];
  /**
   * ID of the handle (omni_handle_private.id reference)
   */
  int id;
  /**
   * Location of the handle (dsa)
   */
  dsa_handle dsa;
  /**
   * Location of the handle (pointer)
   */
  dsa_pointer pointer;
} ModuleEntry;

typedef struct {
  int module_id;
  char name[NAMEDATALEN];
} ModuleAllocationKey;

StaticAssertDecl(sizeof(ModuleAllocationKey) == sizeof(int) + sizeof(char[NAMEDATALEN]),
                 "no padding for ease of hashing");

typedef struct {
  ModuleAllocationKey key;
  dsa_handle dsa_handle;
  dsa_pointer dsa_pointer;
  size_t size;
  pg_atomic_uint32 refcounter;
} ModuleAllocation;

typedef struct {
  dsa_handle handle;
  dsa_area *dsa;
} DSAHandleEntry;

DECLARE_MODULE_VARIABLE(LWLockPadded *locks);

typedef struct {
  const omni_handle *handle;
  void *fn;
  int state_index;
  char *name;
} hook_entry_point;

typedef struct {
  int entry_points_count[__OMNI_HOOK_TYPE_COUNT];
  hook_entry_point *entry_points[__OMNI_HOOK_TYPE_COUNT];
} hook_entry_points_t;

DECLARE_MODULE_VARIABLE(hook_entry_points_t hook_entry_points);

DECLARE_MODULE_VARIABLE(int OMNI_DSA_TRANCHE);

MODULE_FUNCTION void init_backend(void *arg);

MODULE_FUNCTION void load_pending_modules();

MODULE_FUNCTION bool omni_needs_fmgr_hook(Oid fn_oid);

MODULE_FUNCTION void omni_check_password_hook(const char *username, const char *shadow_pass,
                                              PasswordType password_type, Datum validuntil_time,
                                              bool validuntil_null);

MODULE_FUNCTION void omni_executor_start_hook(QueryDesc *queryDesc, int eflags);

MODULE_FUNCTION void omni_executor_run_hook(QueryDesc *queryDesc, ScanDirection direction,
                                            uint64 count, bool execute_once);

MODULE_FUNCTION void omni_executor_finish_hook(QueryDesc *queryDesc);
MODULE_FUNCTION void omni_executor_end_hook(QueryDesc *queryDesc);

MODULE_FUNCTION void omni_process_utility_hook(PlannedStmt *pstmt, const char *queryString,
#if PG_MAJORVERSION_NUM > 13
                                               bool readOnlyTree,
#endif
                                               ProcessUtilityContext context, ParamListInfo params,
                                               QueryEnvironment *queryEnv, DestReceiver *dest,
                                               QueryCompletion *qc);

MODULE_FUNCTION void omni_xact_callback_hook(XactEvent event, void *arg);
MODULE_FUNCTION void omni_emit_log_hook(ErrorData *edata);

DECLARE_MODULE_VARIABLE(void *saved_hooks[__OMNI_HOOK_TYPE_COUNT]);

DECLARE_MODULE_VARIABLE(hook_entry_points_t hook_entry_points);
DECLARE_MODULE_VARIABLE(List *initialized_modules);

#define struct_from_member(s, m, p) ((s *)((char *)(p)-offsetof(s, m)))

MODULE_FUNCTION void default_emit_log(omni_hook_handle *handle, ErrorData *edata);

MODULE_FUNCTION void default_check_password_hook(omni_hook_handle *handle, const char *username,
                                                 const char *shadow_pass,
                                                 PasswordType password_type, Datum validuntil_time,
                                                 bool validuntil_null);

MODULE_FUNCTION bool default_needs_fmgr(omni_hook_handle *handle, Oid fn_oid);

MODULE_FUNCTION void default_executor_start(omni_hook_handle *handle, QueryDesc *queryDesc,
                                            int eflags);

MODULE_FUNCTION void default_executor_run(omni_hook_handle *handle, QueryDesc *queryDesc,
                                          ScanDirection direction, uint64 count, bool execute_once);

MODULE_FUNCTION void default_executor_finish(omni_hook_handle *handle, QueryDesc *queryDesc);

MODULE_FUNCTION void default_executor_end(omni_hook_handle *handle, QueryDesc *queryDesc);

MODULE_FUNCTION void default_process_utility(omni_hook_handle *handle, PlannedStmt *pstmt,
                                             const char *queryString, bool readOnlyTree,
                                             ProcessUtilityContext context, ParamListInfo params,
                                             QueryEnvironment *queryEnv, DestReceiver *dest,
                                             QueryCompletion *qc);

DECLARE_MODULE_VARIABLE(bool backend_force_reload);

MODULE_FUNCTION void unload_module(omni_handle_private *phandle, bool missing_ok);

MODULE_FUNCTION const char *get_omni_library_name();

MODULE_FUNCTION char *get_library_name(const omni_handle *handle);

MODULE_FUNCTION char *get_fitting_library_name(char *library_name);

MODULE_FUNCTION void reorganize_hooks();

DECLARE_MODULE_VARIABLE(MemoryContext OmniGUCContext);

MODULE_FUNCTION dsa_area *dsa_handle_to_area(dsa_handle handle);

MODULE_FUNCTION void extension_upgrade_hook(omni_hook_handle *handle, PlannedStmt *pstmt,
                                            const char *queryString, bool readOnlyTree,
                                            ProcessUtilityContext context, ParamListInfo params,
                                            QueryEnvironment *queryEnv, DestReceiver *dest,
                                            QueryCompletion *qc);

#endif // OMNI_COMMON_H
