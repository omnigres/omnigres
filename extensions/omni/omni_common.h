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
#include <miscadmin.h>
#include <storage/ipc.h>
#include <storage/lwlock.h>
#include <storage/shmem.h>
#include <utils/builtins.h>
#include <utils/hsearch.h>
#include <utils/memutils.h>
#include <utils/rel.h>
#include <utils/syscache.h>

#include <omni.h>

typedef struct {
  volatile pg_atomic_uint32 module_counter;
} omni_shared_info;

extern omni_shared_info *shared_info;

typedef enum {
  OMNI_LOCK_MODULE,
  OMNI_LOCK_ALLOCATION,
  __omni_num_locks,
} omni_locks;

extern HTAB *omni_modules;
extern HTAB *omni_allocations;
extern HTAB *dsa_handles;

typedef enum { HANDLE_LOADED = 0, HANDLE_UNLOADED = 1 } omni_handle_state;

typedef struct {
  /**
   * Handle to share with the module
   */
  omni_handle handle;
  /**
   * Unique handle ID
   */
  uint32 id;
  /**
   * Path the module
   */
  char path[PATH_MAX];
  /**
   *
   */
  volatile pg_atomic_uint32 state;
  /**
   * DSA
   */
  dsa_handle dsa;
} omni_handle_private;

extern omni_handle_private *module_handles;

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
} ModuleEntry;

typedef struct {
  int module_id;
  char name[NAMEDATALEN];
} ModuleAllocationKey;

StaticAssertDecl(sizeof(ModuleAllocationKey) == sizeof(int) + sizeof(char[NAMEDATALEN]),
                 "no padding for ease of hashing");

typedef struct {
  ModuleAllocationKey key;
  int flags;
  dsa_handle dsa_handle;
  dsa_pointer dsa_pointer;
  size_t size;
} ModuleAllocation;

typedef struct {
  dsa_handle handle;
  dsa_area *dsa;
} DSAHandleEntry;

extern LWLock *locks;

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

extern hook_entry_points_t hook_entry_points;

extern int OMNI_DSA_TRANCHE;

static void init_backend(void *arg);

static inline void ensure_backend_initialized(void);
static inline void load_module_if_necessary(Oid fn_oid, bool force_reload);

static bool omni_needs_fmgr_hook(Oid fn_oid);

static void omni_check_password_hook(const char *username, const char *shadow_pass,
                                     PasswordType password_type, Datum validuntil_time,
                                     bool validuntil_null);

static void omni_executor_start_hook(QueryDesc *queryDesc, int eflags);

static void omni_executor_run_hook(QueryDesc *queryDesc, ScanDirection direction, uint64 count,
                                   bool execute_once);

static void omni_executor_finish_hook(QueryDesc *queryDesc);
static void omni_executor_end_hook(QueryDesc *queryDesc);

static void omni_process_utility_hook(PlannedStmt *pstmt, const char *queryString,
#if PG_MAJORVERSION_NUM > 13
                                      bool readOnlyTree,
#endif
                                      ProcessUtilityContext context, ParamListInfo params,
                                      QueryEnvironment *queryEnv, DestReceiver *dest,
                                      QueryCompletion *qc);

static void omni_xact_callback_hook(XactEvent event, void *arg);
static void omni_emit_log_hook(ErrorData *edata);

extern void *saved_hooks[__OMNI_HOOK_TYPE_COUNT];

hook_entry_points_t hook_entry_points;

#define struct_from_member(s, m, p) ((s *)((char *)(p)-offsetof(s, m)))

static void default_emit_log(omni_hook_handle *handle, ErrorData *edata);

static void default_check_password_hook(omni_hook_handle *handle, const char *username,
                                        const char *shadow_pass, PasswordType password_type,
                                        Datum validuntil_time, bool validuntil_null);

static bool default_needs_fmgr(omni_hook_handle *handle, Oid fn_oid);

static void default_executor_start(omni_hook_handle *handle, QueryDesc *queryDesc, int eflags);

static void default_executor_run(omni_hook_handle *handle, QueryDesc *queryDesc,
                                 ScanDirection direction, uint64 count, bool execute_once);

static void default_executor_finish(omni_hook_handle *handle, QueryDesc *queryDesc);

static void default_executor_end(omni_hook_handle *handle, QueryDesc *queryDesc);

static void default_process_utility(omni_hook_handle *handle, PlannedStmt *pstmt,
                                    const char *queryString, bool readOnlyTree,
                                    ProcessUtilityContext context, ParamListInfo params,
                                    QueryEnvironment *queryEnv, DestReceiver *dest,
                                    QueryCompletion *qc);

bool backend_force_reload = false;

void unload_module(int64 id, bool missing_ok);

const char *get_omni_library_name();

#endif // OMNI_COMMON_H
