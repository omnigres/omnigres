#include <string.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <executor/executor.h>
#include <miscadmin.h>
#include <utils/builtins.h>

PG_MODULE_MAGIC;

extern char **environ;

PG_FUNCTION_INFO_V1(environment_variables);

Datum environment_variables(PG_FUNCTION_ARGS) {
  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  char **env = environ;
  for (; *env; env++) {
    char *eq = index(*env, '=');
    Datum values[2] = {PointerGetDatum(cstring_to_text_with_len(*env, eq - *env)),
                       PointerGetDatum(cstring_to_text(eq + 1))};
    bool isnull[2] = {false, false};
    tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
  }

#if PG_MAJORVERSION_NUM < 17
  tuplestore_donestoring(tupstore);
#endif

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}