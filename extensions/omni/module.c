// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <executor/executor.h>
#include <miscadmin.h>
#include <storage/lwlock.h>
#include <utils/rel.h>

#include "omni_common.h"

#include <omni.h>

void _Omni_load(const omni_handle *handle) {}

void _Omni_init(const omni_handle *handle) {}

void _Omni_unload(const omni_handle *handle) {}

PG_FUNCTION_INFO_V1(modules);
Datum modules(PG_FUNCTION_ARGS) {
  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  LWLockAcquire(locks + OMNI_LOCK_MODULE, LW_SHARED);

  HASH_SEQ_STATUS status;
  hash_seq_init(&status, omni_modules);

  ModuleEntry *entry;
  while ((entry = hash_seq_search(&status)) != NULL) {

    Datum values[2] = {Int64GetDatum(entry->id), CStringGetDatum(entry->path)};
    bool isnull[2] = {false, false};
    tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
  }

  LWLockRelease(locks + OMNI_LOCK_MODULE);

  tuplestore_donestoring(tupstore);

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}

char *omni_hook_types[__OMNI_HOOK_TYPE_COUNT] = {[omni_hook_executor_start] = "executor_start",
                                                 [omni_hook_executor_run] = "executor_run",
                                                 [omni_hook_executor_finish] = "executor_finish",
                                                 [omni_hook_executor_end] = "executor_end",
                                                 [omni_hook_needs_fmgr] = "needs_fmgr",
                                                 [omni_hook_process_utility] = "process_utility",
                                                 [omni_hook_emit_log] = "emit_hook",
                                                 [omni_hook_check_password] = "check_password",
                                                 0};

PG_FUNCTION_INFO_V1(hooks);
Datum hooks(PG_FUNCTION_ARGS) {
  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  for (int type = 0; type < __OMNI_HOOK_TYPE_COUNT; type++) {
    for (int i = hook_entry_points.entry_points_count[type] - 1; i >= 0; i--) {
      hook_entry_point *hook = hook_entry_points.entry_points[type] + i;
      omni_handle_private *phandle =
          hook->handle ? struct_from_member(omni_handle_private, handle, hook->handle) : NULL;
      Datum values[4] = {CStringGetDatum(omni_hook_types[type]), CStringGetDatum(hook->name),
                         phandle ? Int64GetDatum(phandle->id) : Int64GetDatum(0),
                         Int32GetDatum(hook_entry_points.entry_points_count[type] - i)};
      bool isnull[4] = {omni_hook_types[type] == NULL, hook->name == NULL, hook->handle == NULL,
                        false};
      tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
    }
  }

  tuplestore_donestoring(tupstore);

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}