// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <commands/user.h>
#include <executor/executor.h>
#include <optimizer/planner.h>

#include "omni_common.h"

MODULE_VARIABLE(void *saved_hooks[__OMNI_HOOK_TYPE_COUNT]);

MODULE_VARIABLE(hook_entry_points_t hook_entry_points);

MODULE_FUNCTION void default_emit_log(omni_hook_handle *handle, ErrorData *edata) {
  return saved_hooks[omni_hook_emit_log] == NULL
             ? false
             : ((emit_log_hook_type)saved_hooks[omni_hook_emit_log])(edata);
}

MODULE_FUNCTION void default_check_password_hook(omni_hook_handle *handle, const char *username,
                                                 const char *shadow_pass,
                                                 PasswordType password_type, Datum validuntil_time,
                                                 bool validuntil_null) {
  return saved_hooks[omni_hook_check_password] == NULL
             ? false
             : ((check_password_hook_type)saved_hooks[omni_hook_check_password])(
                   username, shadow_pass, password_type, validuntil_time, validuntil_null);
}

MODULE_FUNCTION void default_needs_fmgr(omni_hook_handle *handle, Oid fn_oid) {
  handle->returns.bool_value =
      saved_hooks[omni_hook_needs_fmgr] == NULL
          ? false
          : ((needs_fmgr_hook_type)saved_hooks[omni_hook_needs_fmgr])(fn_oid);
}

MODULE_FUNCTION void default_planner(omni_hook_handle *handle, Query *parse,
                                     const char *query_string, int cursorOptions,
                                     ParamListInfo boundParams) {
  // standard_planner() scribbles the `parse` argument.
  // if there are successive calls to standard_planner(),
  // the scribbled `parse` argument is being passed.
  // There is no documentation on exactly what's being scribbled
  // other than looking through the source.
  handle->returns.PlannedStmt_value =
      saved_hooks[omni_hook_planner] == NULL
          ? standard_planner(parse, query_string, cursorOptions, boundParams)
          : ((planner_hook_type)saved_hooks[omni_hook_planner])(parse, query_string, cursorOptions,
                                                                boundParams);
}

MODULE_FUNCTION void default_executor_start(omni_hook_handle *handle, QueryDesc *queryDesc,
                                            int eflags) {
  return saved_hooks[omni_hook_executor_start] == NULL
             ? standard_ExecutorStart(queryDesc, eflags)
             : ((ExecutorStart_hook_type)saved_hooks[omni_hook_executor_start])(queryDesc, eflags);
}

MODULE_FUNCTION void default_executor_run(omni_hook_handle *handle, QueryDesc *queryDesc,
                                          ScanDirection direction, uint64 count,
                                          bool execute_once) {
  return saved_hooks[omni_hook_executor_run] == NULL
             ? standard_ExecutorRun(queryDesc, direction, count, execute_once)
             : ((ExecutorRun_hook_type)saved_hooks[omni_hook_executor_run])(queryDesc, direction,
                                                                            count, execute_once);
}

MODULE_FUNCTION void default_executor_finish(omni_hook_handle *handle, QueryDesc *queryDesc) {
  return saved_hooks[omni_hook_executor_finish] == NULL
             ? standard_ExecutorFinish(queryDesc)
             : ((ExecutorFinish_hook_type)saved_hooks[omni_hook_executor_finish])(queryDesc);
}

MODULE_FUNCTION void default_executor_end(omni_hook_handle *handle, QueryDesc *queryDesc) {
  return saved_hooks[omni_hook_executor_end] == NULL
             ? standard_ExecutorEnd(queryDesc)
             : ((ExecutorEnd_hook_type)saved_hooks[omni_hook_executor_end])(queryDesc);
}

MODULE_FUNCTION void default_process_utility(omni_hook_handle *handle, PlannedStmt *pstmt,
                                             const char *queryString, bool readOnlyTree,
                                             ProcessUtilityContext context, ParamListInfo params,
                                             QueryEnvironment *queryEnv, DestReceiver *dest,
                                             QueryCompletion *qc) {
  return saved_hooks[omni_hook_process_utility] == NULL
             ? standard_ProcessUtility(pstmt, queryString,
#if PG_MAJORVERSION_NUM > 13
                                       readOnlyTree,
#endif
                                       context, params, queryEnv, dest, qc)
             : ((ProcessUtility_hook_type)saved_hooks[omni_hook_process_utility])(
                   pstmt, queryString,
#if PG_MAJORVERSION_NUM > 13
                   readOnlyTree,
#endif
                   context, params, queryEnv, dest, qc);
}

MODULE_FUNCTION void reorganize_hooks() {
  // Remove hooks that are no longer loaded
  for (int type = 0; type < __OMNI_HOOK_TYPE_COUNT; type++) {
    for (int i = 0; i < hook_entry_points.entry_points_count[type]; i++) {
      hook_entry_point *hook = hook_entry_points.entry_points[type] + i;
      if (hook->handle != NULL &&
          !list_member_ptr(initialized_modules,
                           struct_from_member(omni_handle_private, handle, hook->handle))) {
        // This hook is to be removed; we do this by  copying and reindexing successive hooks.
        // We don't reallocate memory as eventual  reallocation would just reuse them to grow if
        // necessary.
        if (hook_entry_points.entry_points_count[type] > i) {
          for (int j = i + 1; j < hook_entry_points.entry_points_count[type]; j++) {
            if (hook_entry_points.entry_points[type][j].state_index >= j) {
              hook_entry_points.entry_points[type][j].state_index--;
            }
            hook_entry_points.entry_points[type][j - 1] = hook_entry_points.entry_points[type][j];
          }
          // After copying, we need to re-run the "unloading test"
          i--;
        }
        // ..and adjusting the count
        hook_entry_points.entry_points_count[type]--;
      }
    }
  }
}

#define iterate_hooks(HOOK, ...)                                                                   \
  ({                                                                                               \
    void *ctxs[hook_entry_points.entry_points_count[omni_hook_##HOOK]];                            \
    omni_hook_return_value retval = {.ptr_value = NULL};                                           \
    if (hook_entry_points.entry_points_count[omni_hook_##HOOK] > 0)                                \
      for (int i = hook_entry_points.entry_points_count[omni_hook_##HOOK] - 1; i >= 0; i--) {      \
        hook_entry_point *hook = hook_entry_points.entry_points[omni_hook_##HOOK] + i;             \
        ctxs[i] = NULL;                                                                            \
        Assert(hook->state_index >= i);                                                            \
        Assert(hook->state_index < hook_entry_points.entry_points_count[omni_hook_##HOOK]);        \
        omni_hook_handle handle = {.handle = hook->handle,                                         \
                                   .ctx = ctxs[hook->state_index],                                 \
                                   .next_action = hook_next_action_next,                           \
                                   .returns = retval};                                             \
        (hook->fn.HOOK)(&handle, __VA_ARGS__);                                                     \
        retval = handle.returns;                                                                   \
        ctxs[i] = handle.ctx;                                                                      \
        switch (handle.next_action) {                                                              \
        case hook_next_action_next:                                                                \
          continue;                                                                                \
        case hook_next_action_finish:                                                              \
          goto done;                                                                               \
        }                                                                                          \
      }                                                                                            \
  done:                                                                                            \
    retval;                                                                                        \
  })

MODULE_FUNCTION bool omni_needs_fmgr_hook(Oid fn_oid) {
  return iterate_hooks(needs_fmgr, fn_oid).bool_value;
}

MODULE_FUNCTION void omni_check_password_hook(const char *username, const char *shadow_pass,
                                              PasswordType password_type, Datum validuntil_time,
                                              bool validuntil_null) {
  iterate_hooks(check_password, username, shadow_pass, password_type, validuntil_time,
                validuntil_null);
}

MODULE_FUNCTION PlannedStmt *omni_planner_hook(Query *parse, const char *query_string,
                                               int cursorOptions, ParamListInfo boundParams) {
  return iterate_hooks(planner, parse, query_string, cursorOptions, boundParams).PlannedStmt_value;
}

MODULE_FUNCTION void omni_executor_start_hook(QueryDesc *queryDesc, int eflags) {
  load_pending_modules();

  iterate_hooks(executor_start, queryDesc, eflags);
}

MODULE_FUNCTION void omni_executor_run_hook(QueryDesc *queryDesc, ScanDirection direction,
                                            uint64 count, bool execute_once) {

  iterate_hooks(executor_run, queryDesc, direction, count, execute_once);
}

MODULE_FUNCTION void omni_executor_finish_hook(QueryDesc *queryDesc) {
  iterate_hooks(executor_finish, queryDesc);
}

MODULE_FUNCTION void omni_executor_end_hook(QueryDesc *queryDesc) {
  iterate_hooks(executor_end, queryDesc);
  load_pending_modules();
}

MODULE_FUNCTION void omni_process_utility_hook(PlannedStmt *pstmt, const char *queryString,
#if PG_MAJORVERSION_NUM > 13
                                               bool readOnlyTree,
#endif
                                               ProcessUtilityContext context, ParamListInfo params,
                                               QueryEnvironment *queryEnv, DestReceiver *dest,
                                               QueryCompletion *qc) {

#if PG_MAJORVERSION_NUM <= 13
  bool readOnlyTree = false;
#endif

  // Prepare this before processing hooks as it may not be available
  // after `PreparedStatement` has been freed.
  bool is_transaction_stmt = nodeTag(pstmt->utilityStmt) != T_TransactionStmt;

  iterate_hooks(process_utility, pstmt, queryString, readOnlyTree, context, params, queryEnv, dest,
                qc);

  // It is critical here to NOT try to rescan modules if we're
  // processing a transaction statement as we may be not having valid invariants for
  // any substantial transactional work (especially if init/deinit callbacks do something
  // with the database). We rely on the next statements to allow us to catch up.
  if (is_transaction_stmt) {
    load_pending_modules();
  }
}

MODULE_VARIABLE(List *xact_oneshot_callbacks);
MODULE_VARIABLE(List *after_xact_oneshot_callbacks);

static void on_xact_dealloc(void *arg) {
  XactEvent event = (XactEvent)(uintptr_t)arg;
  // Hard-coded single-shot hooks
  ListCell *lc;
  // Fire every `after)xact` callback now
  foreach (lc, after_xact_oneshot_callbacks) {
    struct xact_oneshot_callback *cb = (struct xact_oneshot_callback *)lfirst(lc);
    cb->fn(event, cb->arg);
    after_xact_oneshot_callbacks = foreach_delete_current(after_xact_oneshot_callbacks, lc);
  }
  Assert(after_xact_oneshot_callbacks == NIL);
}

MODULE_FUNCTION void omni_xact_callback_hook(XactEvent event, void *arg) {
  iterate_hooks(xact_callback, event);

  // Hard-coded single-shot hooks
  // TODO: these are a bit of a hack and we should find ways to get rid of them

  if (xact_oneshot_callbacks != NIL) {
    ListCell *lc;

    // Fire every `xact` callback now
    foreach (lc, xact_oneshot_callbacks) {
      struct xact_oneshot_callback *cb = (struct xact_oneshot_callback *)lfirst(lc);
      cb->fn(event, cb->arg);
      xact_oneshot_callbacks = foreach_delete_current(xact_oneshot_callbacks, lc);
    }
    Assert(xact_oneshot_callbacks == NIL);
  }

  if (after_xact_oneshot_callbacks != NIL) {
    MemoryContextCallback *cb = MemoryContextAlloc(TopTransactionContext, sizeof(*cb));
    StaticAssertStmt(sizeof(arg) >= sizeof(event), "event should fit into the pointer");
    // Schedule firing `after_xact` callbacks when `TopTransactionContext` is deleted or reset
    cb->func = on_xact_dealloc;
    cb->arg = (void *)event;
    MemoryContextRegisterResetCallback(TopTransactionContext, cb);
  }
}

MODULE_FUNCTION void omni_emit_log_hook(ErrorData *edata) { iterate_hooks(emit_log, edata); }
