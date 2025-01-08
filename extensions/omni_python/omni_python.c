/**
 * \file omni_python.c
 *
 */
// clang-format off
#include <postgres.h>
#include <fmgr.h>
#include <nodes/execnodes.h>
#include <miscadmin.h>
#include <utils/builtins.h>
#include <sys/stat.h>
// clang-format on

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(wheel_paths);

Datum wheel_paths(PG_FUNCTION_ARGS) {

  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  char _pkglib_path[MAXPGPATH];
  get_pkglib_path(my_exec_path, _pkglib_path);

  char *wheel_path = psprintf("%s/omni_python--" EXT_VERSION, _pkglib_path);

  struct stat st;
  if (stat(wheel_path, &st) == 0) {

    tuplestore_putvalues(tupstore, rsinfo->expectedDesc,
                         (Datum[1]){PointerGetDatum(cstring_to_text(wheel_path))},
                         (bool[1]){false});
  }

#if PG_MAJORVERSION_NUM < 17
  tuplestore_donestoring(tupstore);
#endif

  MemoryContextSwitchTo(oldcontext);

  PG_RETURN_NULL();
}